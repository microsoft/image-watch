using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.ComponentModel;
using System.Collections.ObjectModel;
using System.Runtime.CompilerServices;

namespace Microsoft.ImageWatch
{
    public class BindableBase : INotifyPropertyChanged
    {
        protected bool SetProperty<T>(ref T storage, T value,
            [CallerMemberName] string propertyName = "")
        {
            if (!Object.Equals(storage, value))
            {
                storage = value;
                NotifyPropertyChanged(propertyName);
                return true;
            }
            return false;
        }

        public void NotifyAllPropertiesChanged()
        {
            NotifyPropertyChanged("");
        }

        public event PropertyChangedEventHandler PropertyChanged;

        protected void NotifyPropertyChanged(
            [CallerMemberName] String propertyName = "")
        {
            if (PropertyChanged != null)
            {
                PropertyChanged(this,
                    new PropertyChangedEventArgs(propertyName));
            }
        }
    }
}
