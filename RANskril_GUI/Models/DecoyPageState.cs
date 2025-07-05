using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace RANskril_GUI.Models
{
    public enum DecoyDetectionStrategy
    {
        Restrictive,
        Lenient
    }

    public class DecoyPageState : INotifyPropertyChanged
    {
        private int decoyFileRatio = 16;
        public int DecoyFileRatio
        {
            get
            {
                return decoyFileRatio;
            }
            set
            {
                decoyFileRatio = value;
                PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(DecoyFileRatio)));
            }
        }
        private DecoyDetectionStrategy decoyDetection = DecoyDetectionStrategy.Restrictive;
        public DecoyDetectionStrategy DecoyDetection
        {
            get
            {
                return decoyDetection;
            }
            set 
            {
                decoyDetection = value;
                PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(DecoyDetection)));
            }

        }

        public event PropertyChangedEventHandler? PropertyChanged;
    }
}
