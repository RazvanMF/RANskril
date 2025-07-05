using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Navigation;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.Globalization;
using Windows.UI.ViewManagement;
using RANskril_GUI.Utilities;


namespace RANskril_GUI.Pages
{
    public sealed partial class SettingsPage : Page
    {
        public SettingsPage()
        {
            this.InitializeComponent();
        }

        private void ButtonLangEN_Click(object sender, RoutedEventArgs e)
        {
            var command = new ExecutorCommand(ExecutorCommands.DoChangeLanguageEN, sender, e, 0);
            command.Execute();
        }

        private void ButtonLangRO_Click(object sender, RoutedEventArgs e)
        {
            var command = new ExecutorCommand(ExecutorCommands.DoChangeLanguageRO, sender, e, 0);
            command.Execute();
        }

        private void ButtonThemeLight_Click(object sender, RoutedEventArgs e)
        {
            var command = new ExecutorCommand(ExecutorCommands.DoSetThemeLight, sender, e, 0);
            command.Execute();
        }

        private void ButtonThemeDark_Click(object sender, RoutedEventArgs e)
        {
            var command = new ExecutorCommand(ExecutorCommands.DoSetThemeDark, sender, e, 0);
            command.Execute();
        }
    }
}
