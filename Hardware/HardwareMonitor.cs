using LibreHardwareMonitor.Hardware;

namespace BMWClusterMonitor
{
    public class UpdateVisitor : IVisitor
    {
        public void VisitComputer(IComputer computer) { computer.Traverse(this); }
        public void VisitHardware(IHardware hardware)
        {
            hardware.Update();
            foreach (IHardware sub in hardware.SubHardware)
                sub.Accept(this);
        }
        public void VisitSensor(ISensor sensor) { }
        public void VisitParameter(IParameter parameter) { }
    }

    public class HardwareMonitor : IDisposable
    {
        private readonly Computer _computer;
        private readonly UpdateVisitor _visitor = new();

        public HardwareMonitor()
        {
            _computer = new Computer
            {
                IsCpuEnabled = true,
                IsGpuEnabled = true,
                IsMemoryEnabled = true
            };
            _computer.Open();
        }

        public float? GetCpuTemperature()
        {
            _computer.Accept(_visitor);
            foreach (var hw in _computer.Hardware)
                if (hw.HardwareType == HardwareType.Cpu)
                    foreach (var s in hw.Sensors)
                        if (s.SensorType == SensorType.Temperature &&
                            s.Name == "Core (Tctl/Tdie)" &&
                            s.Value.HasValue)
                            return s.Value;
            return null;
        }

        public float? GetCpuUsage()
        {
            _computer.Accept(_visitor);
            foreach (var hw in _computer.Hardware)
                if (hw.HardwareType == HardwareType.Cpu)
                    foreach (var s in hw.Sensors)
                        if (s.SensorType == SensorType.Load &&
                            s.Name == "CPU Total" &&
                            s.Value.HasValue)
                            return s.Value;
            return null;
        }

        public float? GetCpuClock()
        {
            _computer.Accept(_visitor);
            foreach (var hw in _computer.Hardware)
                if (hw.HardwareType == HardwareType.Cpu)
                    foreach (var s in hw.Sensors)
                        if (s.SensorType == SensorType.Clock &&
                            s.Name == "Core #1" &&
                            s.Value.HasValue &&
                            s.Value.Value > 0f)
                            return s.Value;
            return null;
        }

        public float? GetGpuTemperature()
        {
            _computer.Accept(_visitor);
            foreach (var hw in _computer.Hardware)
                if (hw.HardwareType == HardwareType.GpuNvidia)
                    foreach (var s in hw.Sensors)
                        if (s.SensorType == SensorType.Temperature &&
                            s.Name == "GPU Core" &&
                            s.Value.HasValue)
                            return s.Value;
            return null;
        }

        public float? GetGpuHotspot()
        {
            _computer.Accept(_visitor);
            foreach (var hw in _computer.Hardware)
                if (hw.HardwareType == HardwareType.GpuNvidia)
                    foreach (var s in hw.Sensors)
                        if (s.SensorType == SensorType.Temperature &&
                            s.Name == "GPU Hot Spot" &&
                            s.Value.HasValue)
                            return s.Value;
            return null;
        }

        public float? GetGpuClock()
        {
            _computer.Accept(_visitor);
            foreach (var hw in _computer.Hardware)
                if (hw.HardwareType == HardwareType.GpuNvidia)
                    foreach (var s in hw.Sensors)
                        if (s.SensorType == SensorType.Clock &&
                            s.Name == "GPU Core" &&
                            s.Value.HasValue)
                            return s.Value;
            return null;
        }

        public float? GetVramUsed()
        {
            _computer.Accept(_visitor);
            foreach (var hw in _computer.Hardware)
                if (hw.HardwareType == HardwareType.GpuNvidia)
                    foreach (var s in hw.Sensors)
                        if (s.SensorType == SensorType.SmallData &&
                            s.Name == "GPU Memory Used" &&
                            s.Value.HasValue)
                            return s.Value;
            return null;
        }

        public float? GetVramTotal()
        {
            _computer.Accept(_visitor);
            foreach (var hw in _computer.Hardware)
                if (hw.HardwareType == HardwareType.GpuNvidia)
                    foreach (var s in hw.Sensors)
                        if (s.SensorType == SensorType.SmallData &&
                            s.Name == "GPU Memory Total" &&
                            s.Value.HasValue)
                            return s.Value;
            return null;
        }

        public float? GetRamUsed()
        {
            _computer.Accept(_visitor);
            foreach (var hw in _computer.Hardware)
                if (hw.HardwareType == HardwareType.Memory)
                    foreach (var s in hw.Sensors)
                        if (s.SensorType == SensorType.Data &&
                            s.Name == "Memory Used" &&
                            s.Value.HasValue)
                            return s.Value;
            return null;
        }

        public float? GetRamAvailable()
        {
            _computer.Accept(_visitor);
            foreach (var hw in _computer.Hardware)
                if (hw.HardwareType == HardwareType.Memory)
                    foreach (var s in hw.Sensors)
                        if (s.SensorType == SensorType.Data &&
                            s.Name == "Memory Available" &&
                            s.Value.HasValue)
                            return s.Value;
            return null;
        }

        public void Dispose() => _computer.Close();
    }
}