using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Microsoft.UI.Xaml.Documents;
using RANskril_GUI.Middleware;
using RANskril_GUI.Pages;

namespace RANskril_GUI.ViewModel
{
    class ServiceLogViewModel
    {
        public ReadOnlyObservableCollection<Paragraph> Paragraphs { get; }

        public ServiceLogViewModel(ServiceEtwTrace logService)
        {
            Paragraphs = logService.LogParagraphs;
        }
    }
}
