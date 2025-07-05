using System;
using System.Collections.Generic;
using System.IO.Pipes;
using System.Linq;
using System.Reflection.Metadata.Ecma335;
using System.Text;
using System.Threading.Tasks;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.Win32;
using RANskril_GUI.Models;
using RANskril_GUI.Utilities;

namespace RANskril_GUI.Middleware
{
    public class StatusPingPipe
    {
        NamedPipeClientStream pingPipe;
        public StatusPingPipe()
        {
            pingPipe = new NamedPipeClientStream(".", "RANskrilPipeDuplex", PipeDirection.InOut);
            pingPipe.ConnectAsync().ContinueWith(_ => 
            {
                var mainPageState = App.Services.GetRequiredService<MainPageState>();
                bool isRANskrilOn = CheckStatus();
                mainPageState.IsEnabled = isRANskrilOn;
                mainPageState.State = isRANskrilOn ? RANskrilState.Safe : RANskrilState.Off;
            });
        }

        public bool CheckStatus()
        {
            try
            {
                byte[] buffer = new byte[4];
                BitConverter.GetBytes(1).CopyTo(buffer, 0);
                pingPipe.Write(buffer, 0, 4);
                pingPipe.Read(buffer, 0, 4);
                if (BitConverter.IsLittleEndian)
                    buffer.Reverse();

                int status = BitConverter.ToInt32(buffer, 0);
                return status == 0 ? false : true;
            }
            catch
            {
                var mainWindowInstance = App.MainWindow as MainWindow;
                RegistryKey config = Registry.CurrentUser.OpenSubKey(@"Software\RANskril", true);
                string lang = config.GetValue("Language") as string;
                mainWindowInstance.ShowToast("", lang == "en-US" ? RuntimeTranslations.enUSStrings["HandleNoPipeConnection"] : RuntimeTranslations.roROStrings["HandleNoPipeConnection"],
                    Microsoft.UI.Xaml.Controls.InfoBarSeverity.Error);
                return false;
            }
        }

        ~StatusPingPipe()
        {
            pingPipe.Close();
            pingPipe.Dispose();
        }
    }
}
