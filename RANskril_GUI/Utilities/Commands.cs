using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.Win32;
using RANskril_GUI.Middleware;
using RANskril_GUI.Models;
using Windows.Storage;
using Windows.Storage.AccessCache;
using Windows.Storage.Pickers;

namespace RANskril_GUI.Utilities
{
    public enum ExecutorCommands
    {
        None,
        DoRestartSafeMode,
        DoRearmSystem,
        DoChangeLanguageEN,
        DoChangeLanguageRO,
        DoSetThemeLight,
        DoSetThemeDark,
        DoReseedFolders,
        DoResetMetadata,
        DoDisarmSystem,
        DoSetRANskrilState
    }

    public enum OpenerCommands
    {
        None,
        OpenInfringingExecutables,
        OpenExcludedFolders,
        OpenBlindExecutables,
        OpenExcludedExecutables
    }

    public enum AdderCommands
    {
        None,
        AddExcludedFolder,
        AddBlindExecutable,
        AddExcludedExecutable,
    }

    public interface ICommand
    {
        ExecutorCommands ExecCom { get; set; }
        OpenerCommands OpenCom { get; set; }
        AdderCommands AddCom { get; set; }
        void Execute();
    }

    public class ExecutorCommand : ICommand
    {
        public ExecutorCommands ExecCom { get; set; }
        public OpenerCommands OpenCom { get; set; } = OpenerCommands.None;
        public AdderCommands AddCom { get; set; } = AdderCommands.None;

        public object sender;
        public RoutedEventArgs e;
        int differentiator;

        public ExecutorCommand(ExecutorCommands execCom, object sender, RoutedEventArgs e, int differentiator)
        {
            ExecCom = execCom;
            this.sender = sender;
            this.e = e;
            this.differentiator = differentiator;
        }

        async public void Execute()
        {
            var decoyPageState = App.Services.GetRequiredService<DecoyPageState>();
            var senderPipe = App.Services.GetRequiredService<SenderPipe>();
            var mainPageState = App.Services.GetRequiredService<MainPageState>();
            var mainWindowInstance = App.MainWindow as MainWindow;
            if (mainWindowInstance == null)
                return;

            RegistryKey? config = Registry.CurrentUser.OpenSubKey(@"Software\RANskril", true);
            if (config == null)
                return;
            string? lang = config.GetValue("Language") as string;
            if (lang == null)
                lang = "en-US";

            switch (ExecCom)
            {
                case ExecutorCommands.DoRestartSafeMode:
                    mainWindowInstance.ShowToast("", lang == "en-US" ? RuntimeTranslations.enUSStrings["RestartToastInfo"] : RuntimeTranslations.roROStrings["RestartToastInfo"]);
                    senderPipe.Send(0x1000, 0);
                    break;

                case ExecutorCommands.DoRearmSystem:
                    mainWindowInstance.ShowToast("", lang == "en-US" ? RuntimeTranslations.enUSStrings["RearmToastInfo"] : RuntimeTranslations.roROStrings["RearmToastInfo"]);
                    senderPipe.Send(0x2000, 0);

                    mainPageState.State = RANskrilState.Safe;
                    break;

                case ExecutorCommands.DoChangeLanguageEN:
                    config.SetValue("Language", "en-US", RegistryValueKind.String);
                    mainWindowInstance.ShowToast("", lang == "en-US" ? RuntimeTranslations.enUSStrings["ChangeLangGBToastInfo"] : RuntimeTranslations.roROStrings["ChangeLangGBToastInfo"], isRestart: true);
                    break;

                case ExecutorCommands.DoChangeLanguageRO:
                    config.SetValue("Language", "ro-RO", RegistryValueKind.String);
                    mainWindowInstance.ShowToast("", lang == "en-US" ? RuntimeTranslations.enUSStrings["ChangeLangROToastInfo"] : RuntimeTranslations.roROStrings["ChangeLangROToastInfo"], isRestart: true);
                    break;

                case ExecutorCommands.DoSetThemeLight:
                    config.SetValue("Theme", "Light", RegistryValueKind.String);
                    mainWindowInstance.ChangeTheme(ElementTheme.Light);
                    mainWindowInstance.ShowToast("", lang == "en-US" ? RuntimeTranslations.enUSStrings["SetLightThemeToastInfo"] : RuntimeTranslations.roROStrings["SetLightThemeToastInfo"]);
                    break;

                case ExecutorCommands.DoSetThemeDark:
                    config.SetValue("Theme", "Dark", RegistryValueKind.String);
                    mainWindowInstance.ChangeTheme(ElementTheme.Dark);
                    mainWindowInstance.ShowToast("", lang == "en-US" ? RuntimeTranslations.enUSStrings["SetDarkThemeToastInfo"] : RuntimeTranslations.roROStrings["SetDarkThemeToastInfo"]);
                    break;

                case ExecutorCommands.DoReseedFolders:
                    mainWindowInstance.ShowToast("", lang == "en-US" ? RuntimeTranslations.enUSStrings["ReseedFoldersToastInfo"] : RuntimeTranslations.roROStrings["ReseedFoldersToastInfo"], InfoBarSeverity.Warning);
                    senderPipe.Send(0x2, 0);
                    break;

                case ExecutorCommands.DoResetMetadata:
                    mainWindowInstance.ShowToast("", lang == "en-US" ? RuntimeTranslations.enUSStrings["ResetMetadataToastInfo"] : RuntimeTranslations.roROStrings["ResetMetadataToastInfo"]);
                    senderPipe.Send(0x4, 0);
                    break;
                case ExecutorCommands.DoDisarmSystem:
                    mainWindowInstance.ShowToast("", lang == "en-US" ? RuntimeTranslations.enUSStrings["DisarmSystemToastInfo"] : RuntimeTranslations.roROStrings["DisarmSystemToastInfo"], InfoBarSeverity.Warning);
                    senderPipe.Send(0x8, 0);
                    break;

                case ExecutorCommands.DoSetRANskrilState:
                    mainPageState.IsEnabled = differentiator == 0 ? false : true;
                    if (differentiator == 0)
                        mainPageState.State = RANskrilState.Off;
                    else if (differentiator == 1)
                        mainPageState.State = RANskrilState.Safe;
                    senderPipe.Send(0xFFFF, differentiator);
                    break;
            }
        }
    }

    public class OpenerCommand : ICommand
    {
        public ExecutorCommands ExecCom { get; set; } = ExecutorCommands.None;
        public OpenerCommands OpenCom { get; set; }
        public AdderCommands AddCom { get; set; } = AdderCommands.None;

        public OpenerCommand(OpenerCommands openCom, object sender, RoutedEventArgs e, int differentiator)
        {
            OpenCom = openCom;
            this.sender = sender;
            this.e = e;
            this.differentiator = differentiator;
        }

        public object sender;
        public RoutedEventArgs e;
        int differentiator;

        async public void Execute()
        {
            switch (OpenCom)
            {
                case OpenerCommands.OpenInfringingExecutables:
                    string systemRoot = Environment.GetEnvironmentVariable("SystemRoot") ?? "C:\\Windows";
                    string systemTemp = Path.Combine(systemRoot, "Temp");
                    if (File.Exists(systemTemp + "\\ranskril_CRlogs.txt"))
                        Process.Start("notepad.exe", systemTemp + "\\ranskril_CRlogs.txt");
                    else
                    {
                        var mainWindowInstance = App.MainWindow as MainWindow;
                        RegistryKey? config = Registry.CurrentUser.OpenSubKey(@"Software\RANskril", true);
                        if (config == null)
                            return;
                        string? lang = config.GetValue("Language") as string;
                        if (lang == null)
                            lang = "en-US";
                        mainWindowInstance.ShowToast("", lang == "en-US" ? RuntimeTranslations.enUSStrings["HandleNoLog"] : RuntimeTranslations.roROStrings["HandleNoLog"], InfoBarSeverity.Error);
                    }
                    break;
            }
        }
    }


    public class AdderCommand : ICommand
    {
        public ExecutorCommands ExecCom { get; set; } = ExecutorCommands.None;
        public OpenerCommands OpenCom { get; set; } = OpenerCommands.None;
        public AdderCommands AddCom { get; set; }

        public AdderCommand(AdderCommands addCom, object sender, RoutedEventArgs e, int differentiator)
        {
            AddCom = addCom;
            this.sender = sender;
            this.e = e;
            this.differentiator = differentiator;
        }

        public object sender;
        public RoutedEventArgs e;
        int differentiator;

        async public void Execute()
        {
            switch (AddCom)
            {
                case AdderCommands.AddExcludedFolder:
                    System.Diagnostics.Debug.WriteLine("Operation cancelled.");
                    break;
                case AdderCommands.AddBlindExecutable:
                    System.Diagnostics.Debug.WriteLine("Operation cancelled.");
                    break;
                case AdderCommands.AddExcludedExecutable:
                    System.Diagnostics.Debug.WriteLine("Operation cancelled.");
                    break;
            }
        }
    }
}
