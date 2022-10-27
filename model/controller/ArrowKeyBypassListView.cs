using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Controls;
using System.Windows.Input;

namespace DreamCoreV2_model_controller
{
    public class ArrowKeyBypassListView : ListView
    {
        protected override void OnKeyDown(KeyEventArgs e)
        {
            base.OnKeyDown(e);
            
            switch(e.Key)
            {
                case Key.Up:
                case Key.Left:
                case Key.Down:
                case Key.Right:
                    e.Handled = false;
                    break;
            }
        }
    }
}
