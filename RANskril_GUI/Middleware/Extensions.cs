using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;
using Microsoft.UI.Xaml.Documents;

namespace RANskril_GUI.Middleware
{
    public static class Extensions
    {
        public static Paragraph Clone(this Paragraph toClone)
        {
            var clonedPara = new Paragraph();
            foreach (var inline in toClone.Inlines)
            {
                if (inline is Run run)
                {
                    clonedPara.Inlines.Add(new Run
                    {
                        Text = run.Text,
                        FontWeight = run.FontWeight,
                        Foreground = run.Foreground
                    });
                }
            }
            return clonedPara;
        }
    }
}
