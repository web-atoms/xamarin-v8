using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;

namespace DroidV8Test
{
    public class TestResultModel: System.ComponentModel.INotifyPropertyChanged
    {

        public static TestResultModel Model = new TestResultModel();


        private string _Results;
        public string Results
        {
            get { return _Results; }
            set
            {
                _Results = value;
                PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(Results)));
            }
        }

        public event PropertyChangedEventHandler PropertyChanged;
    }
}
