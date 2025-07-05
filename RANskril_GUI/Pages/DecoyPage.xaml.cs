using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Navigation;
using Windows.Foundation;
using Windows.Foundation.Collections;
using RANskril_GUI.Models;
using RANskril_GUI.Utilities;
using Microsoft.Win32;
using System.Security;


namespace RANskril_GUI.Pages
{

    public sealed partial class DecoyPage : Page
    {
        public DecoyPage()
        {
            this.InitializeComponent();

            this.Loaded += (s, e) =>
            {
                DecoyViewer.ItemsSource = ReadRegistry();
            };
        }

        private List<string> ReadRegistry()
        {
            List<string> registry = new List<string>();
            string keyPath = @"SOFTWARE\RANskril\DecoyMon\Paths";
            RegistryKey? decoyHive = Registry.LocalMachine.OpenSubKey(keyPath);
            if (decoyHive == null)
            {
                return registry;
            }
            string[] keyNames = decoyHive.GetSubKeyNames();
            foreach (string keyName in keyNames) 
            {
                string subkeyPath = keyPath + "\\" + keyName;
                using (RegistryKey? subkey = Registry.LocalMachine.OpenSubKey(subkeyPath))
                {
                    if (subkey == null)
                    {
                        continue;
                    }
                    string[] valueNames = subkey.GetValueNames();
                    registry.AddRange(valueNames);
                }
            }
            return registry;
        }

        private void ButtonReseedDecoys_Click(object sender, RoutedEventArgs e)
        {
            var command = new ExecutorCommand(ExecutorCommands.DoReseedFolders, sender, e, 0);
            command.Execute();
        }

        private void ButtonResetMetadata_Click(object sender, RoutedEventArgs e)
        {
            var command = new ExecutorCommand(ExecutorCommands.DoResetMetadata, sender, e, 0);
            command.Execute();
        }

        private void ButtonDisarmSystem_Click(object sender, RoutedEventArgs e)
        {
            var command = new ExecutorCommand(ExecutorCommands.DoDisarmSystem, sender, e, 0);
            command.Execute();
        }
    }
}
