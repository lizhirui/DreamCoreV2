using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Data;

namespace DreamCoreV2_model_controller.Converter
{
    public class LabelTitleAttachConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            if(value is null)
            {
                throw new ArgumentNullException("value is null");
            }

            if(!(value is string))
            {
                throw new ArgumentException("value isn't string");
            }

            if(parameter is null)
            {
                throw new ArgumentNullException("parameter is null");
            }

            if(!(parameter is string))
            {
                throw new ArgumentException("parameter isn't string");
            }

            return (parameter as string) + (value as string); 
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
}
