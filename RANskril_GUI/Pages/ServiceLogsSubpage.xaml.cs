using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Input;
using Microsoft.Diagnostics.Tracing.Session;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.UI;
using Microsoft.UI.Text;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Documents;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Navigation;
using Microsoft.Win32;
using RANskril_GUI.Middleware;
using RANskril_GUI.ViewModel;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.System;

namespace RANskril_GUI.Pages
{
    public sealed partial class ServiceLogsSubpage : Page
    {
        private ServiceLogViewModel serviceLogViewModel;
        private bool autoScroll = true;
        ElementTheme currentTheme = ElementTheme.Light;

        Brush brushInfoLight = new SolidColorBrush(Windows.UI.Color.FromArgb(0xff, 0x1a, 0x1a, 0x1a));
        Brush brushInfoDark = new SolidColorBrush(Windows.UI.Color.FromArgb(0xff, 0xff, 0xff, 0xff));
        Brush brushWarningLight = new SolidColorBrush(Windows.UI.Color.FromArgb(0xff, 0x9d, 0x5d, 0x00));
        Brush brushWarningDark = new SolidColorBrush(Windows.UI.Color.FromArgb(0xff, 0xfc, 0xe1, 0x00));
        Brush brushErrorLight = new SolidColorBrush(Windows.UI.Color.FromArgb(0xff, 0xc4, 0x2b, 0x1c));
        Brush brushErrorDark = new SolidColorBrush(Windows.UI.Color.FromArgb(0xff, 0xff, 0x99, 0xa4));

        public ServiceLogsSubpage()
        {
            this.InitializeComponent();

            serviceLogViewModel = App.Services.GetRequiredService<ServiceLogViewModel>();
            RegistryKey config = Registry.CurrentUser.OpenSubKey(@"Software\RANskril", true);
            string theme = config.GetValue("Theme") as string;
            currentTheme = theme == "Dark" ? ElementTheme.Dark : ElementTheme.Light;

            this.Loaded += (s, e) =>
            {
                RegistryKey config = Registry.CurrentUser.OpenSubKey(@"Software\RANskril", true);
                string theme = config.GetValue("Theme") as string;
                currentTheme = theme == "Dark" ? ElementTheme.Dark : ElementTheme.Light;
                foreach (var para in serviceLogViewModel.Paragraphs)
                {
                    var paraClone = para.Clone();

                    foreach (Inline run in paraClone.Inlines)
                    {
                        Brush chosenBrush = (run as Run).Foreground;
                        if (currentTheme == ElementTheme.Dark)
                        {
                            if ((chosenBrush as SolidColorBrush).Color == (brushInfoLight as SolidColorBrush).Color)
                                chosenBrush = brushInfoDark;
                            if ((chosenBrush as SolidColorBrush).Color == (brushWarningLight as SolidColorBrush).Color)
                                chosenBrush = brushWarningDark;
                            if ((chosenBrush as SolidColorBrush).Color == (brushErrorLight as SolidColorBrush).Color)
                                chosenBrush = brushErrorDark;
                        }
                        run.Foreground = chosenBrush;
                    }

                    ServiceConsole.Blocks.Add(paraClone);
                }

                ((INotifyCollectionChanged)serviceLogViewModel.Paragraphs).CollectionChanged += OnNewLog;

                ServiceConsoleScrollableWrapper.LayoutUpdated += ScrollToLastElement;
            };

            this.Unloaded += (s, e) =>
            {
                ((INotifyCollectionChanged)serviceLogViewModel.Paragraphs).CollectionChanged -= OnNewLog;
                ServiceConsoleScrollableWrapper.LayoutUpdated -= ScrollToLastElement;
                autoScroll = true;
            };
        }

        private void ScrollToLastElement(object? sender, object e)
        {
            if (autoScroll)
                ServiceConsoleScrollableWrapper.ChangeView(0, ServiceConsoleScrollableWrapper.ScrollableHeight, 1);
        }

        private void OnNewLog(object? sender, NotifyCollectionChangedEventArgs e)
        {
            if (e.NewItems is not null)
            {
                foreach (Paragraph para in e.NewItems)
                {
                    var paraClone = para.Clone();

                    foreach (Inline run in paraClone.Inlines)
                    {
                        Brush chosenBrush = (run as Run).Foreground;
                        if (currentTheme == ElementTheme.Dark)
                        {
                            if ((chosenBrush as SolidColorBrush).Color == (brushInfoLight as SolidColorBrush).Color)
                                chosenBrush = brushInfoDark;
                            if ((chosenBrush as SolidColorBrush).Color == (brushWarningLight as SolidColorBrush).Color)
                                chosenBrush = brushWarningDark;
                            if ((chosenBrush as SolidColorBrush).Color == (brushErrorLight as SolidColorBrush).Color)
                                chosenBrush = brushErrorDark;
                        }
                        run.Foreground = chosenBrush;
                    }
                    
                    ServiceConsole.Blocks.Add(paraClone);
                }

                if (autoScroll)
                    ServiceConsoleScrollableWrapper.ChangeView(0, ServiceConsoleScrollableWrapper.ScrollableHeight, 1);
            }
        }

        private void ServiceConsoleScrollableWrapper_PointerEntered(object sender, PointerRoutedEventArgs e)
        {
            autoScroll = false;
        }

        private void ServiceConsoleScrollableWrapper_PointerExited(object sender, PointerRoutedEventArgs e)
        {
            autoScroll = true;
        }

        private void ChangeForegroundOnThemeChange()
        {
            foreach (Paragraph child in ServiceConsole.Blocks)
            {
                child.Foreground = (Brush)App.Current.Resources["ParagraphInfoColor"];
            }
        }
    }
}
