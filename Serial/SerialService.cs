using System.IO.Ports;

namespace BMWClusterMonitor
{
    public class SerialService : IDisposable
    {
        private SerialPort? _port;
        private System.Threading.Timer? _sendTimer;

        public float? CpuTemp { get; set; }
        public float? GpuTemp { get; set; }
        public float? CpuUsage { get; set; }
        public float? CpuClock { get; set; }
        public float? RamUsed { get; set; }
        public float? RamTotal { get; set; }
        public float? GpuClock { get; set; }
        public float? VramUsed { get; set; }
        public float? VramTotal { get; set; }
        public float? GpuHotspot { get; set; }

        public bool IsConnected => _port != null && _port.IsOpen;
        public event Action<string>? OnDataReceived;

        public static string[] GetAvailablePorts() =>
            SerialPort.GetPortNames();

        public bool Connect(string portName)
        {
            try
            {
                _port = new SerialPort(portName, 9600)
                {
                    NewLine = "\n",
                    WriteTimeout = 500,
                    ReadTimeout = 500
                };
                _port.DataReceived += Port_DataReceived;
                _port.Open();

                _sendTimer = new System.Threading.Timer(
                    SendData, null,
                    TimeSpan.Zero,
                    TimeSpan.FromMilliseconds(1000)
                );
                return true;
            }
            catch { _port = null; return false; }
        }

        private void Port_DataReceived(object sender,
            SerialDataReceivedEventArgs e)
        {
            try
            {
                if (_port == null || !_port.IsOpen) return;
                string data = _port.ReadExisting();
                if (!string.IsNullOrWhiteSpace(data))
                    OnDataReceived?.Invoke(data.Trim());
            }
            catch { }
        }

        private void SendData(object? state)
        {
            if (_port == null || !_port.IsOpen) return;
            try
            {
                float cpu = CpuTemp ?? 0f;
                float gpu = GpuTemp ?? 0f;
                float cpuu = CpuUsage ?? 0f;
                float cpuc = CpuClock ?? 0f;
                float ramu = RamUsed ?? 0f;
                float ramt = RamTotal ?? 0f;
                float gpuc = GpuClock ?? 0f;
                float vram = VramUsed ?? 0f;
                float vramm = VramTotal ?? 0f;
                float ghot = GpuHotspot ?? 0f;

                string message =
                    $"CPU={cpu:F0}," +
                    $"GPU={gpu:F0}," +
                    $"CPUU={cpuu:F0}," +
                    $"CPUC={cpuc:F0}," +
                    $"RAMU={ramu:F1}," +
                    $"RAMT={ramt:F1}," +
                    $"GPUC={gpuc:F0}," +
                    $"VRAM={vram:F0}," +
                    $"VRAMM={vramm:F0}," +
                    $"GHOT={ghot:F0}";

                _port.WriteLine(message);
            }
            catch { }
        }

        public void Disconnect()
        {
            _sendTimer?.Dispose();
            _sendTimer = null;

            if (_port != null)
            {
                _port.DataReceived -= Port_DataReceived;
                if (_port.IsOpen) _port.Close();
                _port.Dispose();
                _port = null;
            }
        }

        public void Dispose() => Disconnect();
    }
}