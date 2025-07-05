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


namespace RANskril_GUI.Pages
{
    public sealed partial class LogsPage : Page
    {
        public LogsPage()
        {
            this.InitializeComponent();
        }

        private void LogNavView_ItemInvoked(NavigationView sender, NavigationViewItemInvokedEventArgs args)
        {
            var invokedItemContainer = args.InvokedItemContainer as NavigationViewItem;
            switch (invokedItemContainer.Tag)
            {
                case "servlogs":
                    LogContentFrame.Navigate(typeof(Pages.ServiceLogsSubpage));
                    break;
                case "fltlogs":
                    LogContentFrame.Navigate(typeof(Pages.KernelLogsSubpage));
                    break;
            }
        }

        private void LogNavView_Loaded(object sender, RoutedEventArgs e)
        {
            LogNavView.SelectedItem = LogNavView.MenuItems[0];
            LogContentFrame.Navigate(typeof(Pages.ServiceLogsSubpage));
        }
    }
}
