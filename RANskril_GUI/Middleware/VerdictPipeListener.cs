using System;
using System.Collections.Generic;
using System.IO.Pipes;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using RANskril_GUI.Models;

namespace RANskril_GUI.Middleware
{
    class VerdictPipeListener
    {
        MainPageState mainPageState;
        public NamedPipeClientStream receiverPipe;
        public VerdictPipeListener(MainPageState mainPageState)
        {
            this.mainPageState = mainPageState;
            receiverPipe = new NamedPipeClientStream(".", "RANskrilPipeOut", PipeDirection.In);
            Task.Run(StartListening);
        }

        void StartListening()
        {
            System.Diagnostics.Debug.WriteLine("STARTING LISTENER THREAD...");
            receiverPipe.Connect();
            System.Diagnostics.Debug.WriteLine("CONNECTED TO LISTENER PIPE...");
            while (receiverPipe.IsConnected)
            {
                byte[] value = new byte[4];
                receiverPipe.Read(value, 0, 4);

                System.Diagnostics.Debug.WriteLine("READ FROM LISTENER PIPE...");
                if (BitConverter.IsLittleEndian)
                    value.Reverse();

                int verdict = BitConverter.ToInt32(value, 0);
                System.Diagnostics.Debug.WriteLine($"GOT {verdict}!");
                if (mainPageState.State == RANskrilState.Safe)
                    App.MainDispatcherQueue.TryEnqueue(() => { mainPageState.State = (verdict == 0) ? RANskrilState.Safe : RANskrilState.Tripped; });
                    
            }
        }

        public void TerminateConnection()
        {
            receiverPipe?.Close();
            receiverPipe?.Dispose();
        }
    }
}
