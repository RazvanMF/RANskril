using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Xml.Linq;
using Microsoft.Diagnostics.Tracing;
using Microsoft.Diagnostics.Tracing.Session;
using Microsoft.UI.Text;
using Microsoft.UI.Xaml.Documents;
using Microsoft.UI.Xaml.Media;

namespace RANskril_GUI.Middleware
{
    public class ServiceEtwTrace
    {
        private readonly ObservableCollection<Paragraph> _logParagraphs = new();
        public ReadOnlyObservableCollection<Paragraph> LogParagraphs { get; }

        public ServiceEtwTrace()
        {
            LogParagraphs = new(_logParagraphs);
            Task.Run(StartListening);
        }

        private void StartListening()
        {
            // RANskrilKernelLogger trace: 4b8d3d0e-4a4e-4405-a169-7064620b632a
            Guid providerGuid = new("4b8d3d0e-4a4e-4405-a169-7064620b632a");

            Task.Run(() =>
            {
                try
                {
                    using var session = new TraceEventSession("RANskrilServiceListener");
                    session.StopOnDispose = true;

                    session.EnableProvider(providerGuid);

                    session.Source.Dynamic.All += traceEvent =>
                    {
                        var time = traceEvent?.TimeStamp.ToString("T") ?? "??:??:??";
                        var level = traceEvent?.Level.ToString() ?? "N/A";

                        Windows.UI.Color coloring = Windows.UI.Color.FromArgb(0xff, 0x1a, 0x1a, 0x1a);
                        switch (traceEvent?.Level)
                        {
                            case TraceEventLevel.Informational:
                                coloring = Windows.UI.Color.FromArgb(0xff, 0x1a, 0x1a, 0x1a);
                                break;
                            case TraceEventLevel.Warning:
                                coloring = Windows.UI.Color.FromArgb(0xff, 0x9d, 0x5d, 0x00);
                                break;
                            case TraceEventLevel.Error:
                                coloring = Windows.UI.Color.FromArgb(0xff, 0xc4, 0x2b, 0x1c);
                                break;
                        }

                        string payload = "", message = "", subcomponent = "";
                        foreach (var name in traceEvent.PayloadNames)
                        {
                            if (name == "Message")
                                message = traceEvent.PayloadByName(name).ToString();
                            else
                                subcomponent = traceEvent.PayloadByName(name).ToString();
                        }
                        payload = subcomponent + " | " + message;

                        App.MainDispatcherQueue.TryEnqueue(() =>
                        {
                            var para = new Paragraph();
                            para.Inlines.Add(new Run
                            {
                                Text = $"[{time} | {level}] {payload}",
                                FontWeight = FontWeights.Normal,
                                Foreground = new SolidColorBrush(coloring)
                            });

                            _logParagraphs.Add(para);
                        });
                    };

                    session.Source.Process();
                }
                catch (Exception ex)
                {
                    App.MainDispatcherQueue.TryEnqueue(() =>
                    {
                        var errorPara = new Paragraph();
                        errorPara.Inlines.Add(new Run
                        {
                            Text = $"[ETW ERROR] {ex}",
                            FontWeight = FontWeights.Bold,
                            Foreground = new SolidColorBrush(Windows.UI.Color.FromArgb(0xff, 0xc4, 0x2b, 0x1c))
                        });
                        _logParagraphs.Add(errorPara);
                    });
                }
            });
        }
    }
}
