using System;
using System.Collections.Generic;
using System.IO.Pipes;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Microsoft.Win32;
using RANskril_GUI.Models;
using RANskril_GUI.Utilities;

namespace RANskril_GUI.Middleware
{
    class SenderPipe
    {
        NamedPipeClientStream senderPipe;

        public SenderPipe()
        {
            senderPipe = new NamedPipeClientStream(".", "RANskrilPipeIn", PipeDirection.Out);
            senderPipe.ConnectAsync();
        }

        public void Send(int param1, int param2)
        {
            try 
            {
                byte[] buffer = new byte[8];
                BitConverter.GetBytes(param1).CopyTo(buffer, 0);
                BitConverter.GetBytes(param2).CopyTo(buffer, 4);
                senderPipe.Write(buffer, 0, 8);
            }
            catch
            {
                var mainWindowInstance = App.MainWindow as MainWindow;
                RegistryKey config = Registry.CurrentUser.OpenSubKey(@"Software\RANskril", true);
                string lang = config.GetValue("Language") as string;
                mainWindowInstance.ShowToast("", lang == "en-US" ? RuntimeTranslations.enUSStrings["HandleNoPipeConnection"] : RuntimeTranslations.roROStrings["HandleNoPipeConnection"], 
                    Microsoft.UI.Xaml.Controls.InfoBarSeverity.Error);
            }
        }

        ~SenderPipe() 
        {
            senderPipe.Close();
            senderPipe.Dispose();
        }
    }
}
