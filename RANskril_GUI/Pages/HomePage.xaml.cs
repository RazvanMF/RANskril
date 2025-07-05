using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.ComponentModel;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Microsoft.Diagnostics.Tracing.Parsers.ApplicationServer;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Media.Imaging;
using Microsoft.UI.Xaml.Navigation;
using Microsoft.Win32;
using RANskril_GUI.Middleware;
using RANskril_GUI.Models;
using RANskril_GUI.Utilities;
using RANskril_GUI.ViewModel;
using Windows.ApplicationModel.Resources;
using Windows.Foundation;
using Windows.Foundation.Collections;

namespace RANskril_GUI.Pages
{
    public sealed partial class HomePage : Page
    {
        private MainPageState mainPageState;
        Dictionary<string, string> enUSText = new() { {"off", "RANskril is turned off." }, {"safe", "No anomalies detected! Your computer is safe." }, { "unsafe", "The system was tripped. Investigate immediately!" } };
        Dictionary<string, string> roROText = new() { {"off", "RANskril nu este pornit." }, {"safe", "Nu au fost detectate anomalii. Computer-ul dvs. este în siguranță." }, { "unsafe", "Sistemul a fost declanșat. Vă rugăm să investigați!" } };

        private PropertyChangedEventHandler? stateHandler;

        public HomePage()
        {
            this.InitializeComponent();
            mainPageState = App.Services.GetRequiredService<MainPageState>();
            var verdictPipeListener = App.Services.GetRequiredService<VerdictPipeListener>();
            var statusPingPipeListener = App.Services.GetRequiredService<StatusPingPipe>();


            this.Loaded += (s, e) =>
            {
                ReloadState();
                stateHandler = (s, e) => ReloadState();
                mainPageState.PropertyChanged += stateHandler;
                ToggleSwitch.Toggled += ToggleSwitch_Toggled;
            };

            this.Unloaded += (s, e) =>
            {
                mainPageState.PropertyChanged -= stateHandler;
                ToggleSwitch.Toggled -= ToggleSwitch_Toggled;
            };
        }

        private void ReloadState()
        {
            RegistryKey config = Registry.CurrentUser.OpenSubKey(@"Software\RANskril");
            var lang = config.GetValue("Language") as string;
            var theme = config.GetValue("Theme") as string;
            string text = (lang == "en-US" ? enUSText["off"] : roROText["off"]);

            ToggleSwitch.IsOn = mainPageState.IsEnabled;

            var assembly = typeof(Program).Assembly;
            var stream = assembly.GetManifestResourceStream("RANskril_GUI.Assets.mathematics-sign-minus-outline-icon.png");
            ImageSource img = new BitmapImage();
            BitmapImage temp = new BitmapImage();
            temp.SetSource(stream.AsRandomAccessStream());
            

            switch (mainPageState.State)
            {
                case RANskrilState.Off:
                    text = (lang == "en-US" ? enUSText["off"] : roROText["off"]);
                    stream = assembly.GetManifestResourceStream("RANskril_GUI.Assets.mathematics-sign-minus-outline-icon.png");
                    temp.SetSource(stream.AsRandomAccessStream());
                    ButtonInfringe.IsEnabled = false;
                    ButtonRearm.IsEnabled = false;
                    ButtonRestart.IsEnabled = false;
                    break;
                case RANskrilState.Safe:
                    text = (lang == "en-US" ? enUSText["safe"] : roROText["safe"]);
                    stream = assembly.GetManifestResourceStream("RANskril_GUI.Assets.green-checkmark-line-icon.png");
                    temp.SetSource(stream.AsRandomAccessStream());
                    ButtonInfringe.IsEnabled = false;
                    ButtonRearm.IsEnabled = false;
                    ButtonRestart.IsEnabled = false;
                    break;
                case RANskrilState.Tripped:
                    text = (lang == "en-US" ? enUSText["unsafe"] : roROText["unsafe"]);
                    stream = assembly.GetManifestResourceStream("RANskril_GUI.Assets.red-x-line-icon.png");
                    temp.SetSource(stream.AsRandomAccessStream());
                    ButtonInfringe.IsEnabled = true;
                    ButtonRearm.IsEnabled = true;
                    ButtonRestart.IsEnabled = true;
                    ToggleSwitch.IsEnabled = false;
                    break;
            }
            img = temp;
            statusText.Text = text;
            statusImage.Source = img;
        }

        private void ButtonInfringe_Click(object sender, RoutedEventArgs e)
        {
            var command = new OpenerCommand(OpenerCommands.OpenInfringingExecutables, sender, e, 0);
            command.Execute();
        }

        private void ButtonRestart_Click(object sender, RoutedEventArgs e)
        {
            var command = new ExecutorCommand(ExecutorCommands.DoRestartSafeMode, sender, e, 0);
            command.Execute();
        }

        private void ButtonRearm_Click(object sender, RoutedEventArgs e)
        {
            var command = new ExecutorCommand(ExecutorCommands.DoRearmSystem, sender, e, 0);
            command.Execute();

            ToggleSwitch.IsEnabled = true;
        }

        private void ToggleSwitch_Toggled(object sender, RoutedEventArgs e)
        {
            var command = new ExecutorCommand(ExecutorCommands.DoSetRANskrilState, sender, e, ToggleSwitch.IsOn == true ? 1 : 0);
            command.Execute();
        }
    }
}
