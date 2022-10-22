using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace DreamCoreV2_model_controller.DataSource
{
    public class ObjectDataSource<T> : INotifyPropertyChanged where T : class
    {
        public event PropertyChangedEventHandler? PropertyChanged;
        private T? _value = null;

        public T? Value
        {
            get
            {
                return _value;
            }

            set
            {
                _value = value;
                PropertyChanged?.Invoke(this, new PropertyChangedEventArgs("Value"));
            }
        }
        
        ObjectDataSource(T? value)
        {
            Value = value;
        }

        public static implicit operator ObjectDataSource<T>(T? v)
        {
            return new ObjectDataSource<T>(v);
        }

        public static implicit operator T?(ObjectDataSource<T> v)
        {
            return v.Value;
        }

        public T getNotNull()
        {
            if(_value == null)
            {
                throw new NullReferenceException("value is null!");
            }

            return _value;
        }
    }
}
