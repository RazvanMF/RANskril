using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.UI.Dispatching;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Navigation;
using Microsoft.UI.Xaml.Shapes;
using Microsoft.Windows.Globalization;
using Windows.ApplicationModel;
using Windows.ApplicationModel.Activation;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.Storage;
using Windows.UI.ViewManagement;
using RANskril_GUI.Middleware;
using RANskril_GUI.Models;
using RANskril_GUI.ViewModel;
using Microsoft.Win32;
using System.Runtime.CompilerServices;


namespace RANskril_GUI
{
    public partial class App : Application
    {

        public static DispatcherQueue MainDispatcherQueue { get; set; }
        public static Window? MainWindow { get; private set; }
        public static IServiceProvider Services { get; set; }

        public App()
        {
            RegistryKey? config = Registry.CurrentUser.OpenSubKey(@"Software\RANskril", true);
            if (config == null)
            {
                config = Registry.CurrentUser.CreateSubKey(@"Software\RANskril");
                config.SetValue("Language", "en-US", RegistryValueKind.String);
                config.SetValue("Theme", "Light", RegistryValueKind.String);
            }
            string language = config.GetValue("Language") as string;

            ApplicationLanguages.PrimaryLanguageOverride = language;
            this.InitializeComponent();
            Services = ConfigureServices();
        }

        protected override void OnLaunched(Microsoft.UI.Xaml.LaunchActivatedEventArgs args)
        {

            MainWindow = new MainWindow();
            MainWindow.Closed += (s, e) =>
            {
                var verdictPipeListener = App.Services.GetRequiredService<VerdictPipeListener>();
                verdictPipeListener.TerminateConnection();
            };

            MainWindow.Activate();
        }

        private Window? m_window;


        private static IServiceProvider ConfigureServices()
        {
            var services = new ServiceCollection();

            // Register services
            services.AddSingleton<ServiceEtwTrace>();
            services.AddSingleton<KernelEtwTrace>();

            services.AddSingleton<ServiceLogViewModel>();
            services.AddSingleton<KernelLogViewModel>();

            services.AddSingleton<MainPageState>();
            services.AddSingleton<DecoyPageState>();

            services.AddSingleton<VerdictPipeListener>();
            services.AddSingleton<SenderPipe>();
            services.AddSingleton<StatusPingPipe>();

            return services.BuildServiceProvider();
        }
    }
}
