using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Microsoft.UI.Xaml.Documents;
using RANskril_GUI.Middleware;

namespace RANskril_GUI.ViewModel
{
    class KernelLogViewModel
    {
        public ReadOnlyObservableCollection<Paragraph> Paragraphs { get; }

        public KernelLogViewModel(KernelEtwTrace logService)
        {
            Paragraphs = logService.LogParagraphs;
        }
    }
}
