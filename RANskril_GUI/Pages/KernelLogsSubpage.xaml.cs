using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Microsoft.Extensions.DependencyInjection;
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


namespace RANskril_GUI.Pages
{
    public sealed partial class KernelLogsSubpage : Page
    {
        private KernelLogViewModel kernelLogViewModel;
        private bool autoScroll = true;
        ElementTheme currentTheme = ElementTheme.Light;

        Brush brushInfoLight = new SolidColorBrush(Windows.UI.Color.FromArgb(0xff, 0x1a, 0x1a, 0x1a));
        Brush brushInfoDark = new SolidColorBrush(Windows.UI.Color.FromArgb(0xff, 0xff, 0xff, 0xff));
        Brush brushWarningLight = new SolidColorBrush(Windows.UI.Color.FromArgb(0xff, 0x9d, 0x5d, 0x00));
        Brush brushWarningDark = new SolidColorBrush(Windows.UI.Color.FromArgb(0xff, 0xfc, 0xe1, 0x00));
        Brush brushErrorLight = new SolidColorBrush(Windows.UI.Color.FromArgb(0xff, 0xc4, 0x2b, 0x1c));
        Brush brushErrorDark = new SolidColorBrush(Windows.UI.Color.FromArgb(0xff, 0xff, 0x99, 0xa4));

        public KernelLogsSubpage()
        {
            this.InitializeComponent();

            kernelLogViewModel = App.Services.GetRequiredService<KernelLogViewModel>();
            RegistryKey config = Registry.CurrentUser.OpenSubKey(@"Software\RANskril", true);
            string theme = config.GetValue("Theme") as string;
            currentTheme = theme == "Dark" ? ElementTheme.Dark : ElementTheme.Light;

            this.Loaded += (s, e) =>
            {
                RegistryKey config = Registry.CurrentUser.OpenSubKey(@"Software\RANskril", true);
                string theme = config.GetValue("Theme") as string;
                currentTheme = theme == "Dark" ? ElementTheme.Dark : ElementTheme.Light;
                foreach (var para in kernelLogViewModel.Paragraphs)
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

                    KernelConsole.Blocks.Add(paraClone);
                }

                ((INotifyCollectionChanged)kernelLogViewModel.Paragraphs).CollectionChanged += OnNewLog;

                KernelConsoleScrollableWrapper.LayoutUpdated += ScrollToLastElement;
            };

            this.Unloaded += (s, e) =>
            {
                ((INotifyCollectionChanged)kernelLogViewModel.Paragraphs).CollectionChanged -= OnNewLog;
                KernelConsoleScrollableWrapper.LayoutUpdated -= ScrollToLastElement;
                autoScroll = true;
            };
        }

        private void ScrollToLastElement(object? sender, object e)
        {
            if (autoScroll)
                KernelConsoleScrollableWrapper.ChangeView(0, KernelConsoleScrollableWrapper.ScrollableHeight, 1);
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

                    KernelConsole.Blocks.Add(paraClone);
                }

                if (autoScroll)
                    KernelConsoleScrollableWrapper.ChangeView(0, KernelConsoleScrollableWrapper.ScrollableHeight, 1);
            }
        }

        private void KernelConsoleScrollableWrapper_PointerEntered(object sender, PointerRoutedEventArgs e)
        {
            autoScroll = false;
        }

        private void KernelConsoleScrollableWrapper_PointerExited(object sender, PointerRoutedEventArgs e)
        {
            autoScroll = true;
        }
    }
}
