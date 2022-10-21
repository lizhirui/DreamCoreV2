using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Input;

namespace DreamCoreV2_model_controller
{
    public static class CustomCommands
    {
        public static RoutedCommand Continue = new RoutedCommand();
        public static RoutedCommand Pause = new RoutedCommand();
        public static RoutedCommand StepCommit = new RoutedCommand();
        public static RoutedCommand Step = new RoutedCommand();
        public static RoutedCommand Reset = new RoutedCommand();
    }
}
