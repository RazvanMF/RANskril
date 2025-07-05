using System;
using System.Collections.Generic;
using System.Diagnostics;
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
using Microsoft.Win32;
using Microsoft.Windows.Storage;
using RANskril_GUI.Middleware;
using RANskril_GUI.Utilities;
using RANskril_GUI.ViewModel;
using Windows.Foundation;
using Windows.Foundation.Collections;


namespace RANskril_GUI
{
    public sealed partial class MainWindow : Window
    {
        public MainWindow()
        {
            this.InitializeComponent();
            RegistryKey config = Registry.CurrentUser.OpenSubKey(@"Software\RANskril", true);
            string theme = config.GetValue("Theme") as string;
            RootGrid.RequestedTheme = theme == "Dark" ? ElementTheme.Dark : ElementTheme.Light;
            App.MainDispatcherQueue = this.DispatcherQueue;
            var serviceLogLoader = App.Services.GetRequiredService<ServiceLogViewModel>();
            var kernelLogLoader = App.Services.GetRequiredService<KernelLogViewModel>();
            var verdictPipeListener = App.Services.GetRequiredService<VerdictPipeListener>();
            var senderPipe = App.Services.GetRequiredService<SenderPipe>();

        }

        private void nvSample_ItemInvoked(NavigationView sender, NavigationViewItemInvokedEventArgs args)
        {
            if (args.IsSettingsInvoked)
            {
                contentFrame.Navigate(typeof(Pages.SettingsPage));
                return;
            }
            var invokedItemContainer = args.InvokedItemContainer as NavigationViewItem;
            switch(invokedItemContainer.Tag)
            {
                case "home":
                    contentFrame.Navigate(typeof(Pages.HomePage));
                    break;
                case "logs":
                    contentFrame.Navigate(typeof(Pages.LogsPage));
                    break;
                case "decoys":
                    contentFrame.Navigate(typeof(Pages.DecoyPage));
                    break;
                case "recovery":
                    contentFrame.Navigate(typeof(Pages.RecoveryPage));
                    break;
            }
            
        }

        private void nvSample_Loaded(object sender, RoutedEventArgs e)
        {
            nvSample.SelectedItem = nvSample.MenuItems[0];
            contentFrame.Navigate(typeof(Pages.HomePage));
        }

        public void ShowToast(string title, string message, InfoBarSeverity infoBarSeverity = InfoBarSeverity.Informational, int timeout = 5, bool isRestart = false)
        {
            InfoBar infoBar = new InfoBar 
            {
                Title = title,
                Severity = infoBarSeverity,
                Message = message,
                HorizontalAlignment = HorizontalAlignment.Stretch,
                Opacity = 0,
                OpacityTransition = new ScalarTransition(),
            };

            if (isRestart)
            {
                var mainWindowInstance = App.MainWindow as MainWindow;
                RegistryKey? config = Registry.CurrentUser.OpenSubKey(@"Software\RANskril", true);
                if (config == null)
                    return;
                string? lang = config.GetValue("Language") as string;
                if (lang == null)
                    lang = "en-US";

                Button restartButton = new Button();
                restartButton.Content = lang == "en-US" ? "Restart" : "Repornește" ;
                restartButton.Click += RestartButton_Click;
                infoBar.ActionButton = restartButton;
                
            }

            ToastHost.Children.Add(infoBar);
            
            infoBar.IsOpen = true;
            DispatcherQueue.TryEnqueue(() => infoBar.Opacity = 1);

            infoBar.Closed += (s, e) =>
            {
                ToastHost.Children.Remove(infoBar);
            };

            var opacityTimer = new DispatcherTimer { Interval = TimeSpan.FromSeconds(timeout) };
            opacityTimer.Tick += (s, e) =>
            {
                opacityTimer.Stop();
                infoBar.Opacity = 0;
                var closeTimer = new DispatcherTimer { Interval = TimeSpan.FromSeconds(1) };
                closeTimer.Tick += (s, e) =>
                {
                    closeTimer.Stop();
                    infoBar.IsOpen = false;
                };
                DispatcherQueue.TryEnqueue(() => closeTimer.Start());
            };

            DispatcherQueue.TryEnqueue(() => opacityTimer.Start());
        }

        private void RestartButton_Click(object sender, RoutedEventArgs e)
        {
            ProcessStartInfo Info = new ProcessStartInfo();
            Info.UseShellExecute = true;
            Info.Arguments = "/C ping 127.0.0.1 -n 2 && \"" + Environment.ProcessPath + "\"";  // https://stackoverflow.com/questions/9603926/restart-an-application-by-itself, kudos
            Info.WindowStyle = ProcessWindowStyle.Hidden;
            Info.CreateNoWindow = true;
            Info.FileName = "cmd.exe";
            Process.Start(Info);
            Environment.Exit(0);
        }

        public void ChangeTheme(ElementTheme theme)
        {
            RootGrid.RequestedTheme = theme;
        }
    }
}
