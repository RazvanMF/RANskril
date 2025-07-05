using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace RANskril_GUI.Models
{
    public enum RANskrilState
    {
        Off,
        Safe,
        Tripped
    }

    class MainPageState : INotifyPropertyChanged
    {
        private RANskrilState state = RANskrilState.Safe;
        private bool isEnabled = true;
        public RANskrilState State
        {
            get
            {
                return state;
            }
            set
            {
                state = value;
                PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(State)));
            }
        }

        public bool IsEnabled
        {
            get
            {
                return isEnabled;
            }
            set
            {
                isEnabled = value;
                PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(IsEnabled)));
            }
        }

        public event PropertyChangedEventHandler? PropertyChanged;
    }
}
