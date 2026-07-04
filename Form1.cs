using System.IO;

namespace BMWClusterMonitor
{
    public partial class Form1 : Form
    {
        private readonly HardwareMonitor _monitor = new();
        private readonly SerialService _serial = new();
        private readonly System.Windows.Forms.Timer _timer = new();

        public Form1()
        {
            InitializeComponent();
            RefreshComPorts();

            _timer.Interval = 1000;
            _timer.Tick += Timer_Tick;
            _timer.Start();

            UpdateTemperatures();
        }
        protected override void OnFormClosing(FormClosingEventArgs e)
        {
            if (e.CloseReason == CloseReason.UserClosing)
            {
                // Instead of closing, hide to tray
                e.Cancel = true;
                Hide();
                trayIcon.ShowBalloonTip(1000, "BMW Cluster Monitor",
                    "Still running in the background.", ToolTipIcon.Info);
            }
            else
            {
                base.OnFormClosing(e);
            }
        }

        private void RefreshComPorts()
        {
            cmbPorts.Items.Clear();
            var ports = SerialService.GetAvailablePorts();

            if (ports.Length == 0)
            {
                cmbPorts.Items.Add("No ports found");
                cmbPorts.SelectedIndex = 0;
                btnConnect.Enabled = false;
            }
            else
            {
                foreach (var port in ports)
                    cmbPorts.Items.Add(port);
                cmbPorts.SelectedIndex = 0;
                btnConnect.Enabled = true;
            }
        }

        private void btnConnect_Click(object sender, EventArgs e)
        {
            if (_serial.IsConnected)
            {
                _serial.Disconnect();
                btnConnect.Text = "Connect";
                cmbPorts.Enabled = true;
                btnRefresh.Enabled = true;
                lblStatus.Text = "Status: Disconnected";
                lblStatus.ForeColor = System.Drawing.Color.Red;
            }
            else
            {
                string selectedPort =
                    cmbPorts.SelectedItem?.ToString() ?? "";

                if (string.IsNullOrEmpty(selectedPort) ||
                    selectedPort == "No ports found")
                {
                    MessageBox.Show("Please select a valid COM port.");
                    return;
                }

                _serial.OnDataReceived += Serial_OnDataReceived;
                bool success = _serial.Connect(selectedPort);

                if (success)
                {
                    btnConnect.Text = "Disconnect";
                    cmbPorts.Enabled = false;
                    btnRefresh.Enabled = false;
                    lblStatus.Text =
                        $"Status: Connected on {selectedPort}";
                    lblStatus.ForeColor = System.Drawing.Color.Green;
                }
                else
                {
                    _serial.OnDataReceived -= Serial_OnDataReceived;
                    MessageBox.Show(
                        $"Could not open {selectedPort}.\n\n" +
                        "• Make sure Arduino is plugged in\n" +
                        "• Close Arduino IDE completely\n" +
                        "• Try unplugging and replugging USB",
                        "Connection Failed",
                        MessageBoxButtons.OK,
                        MessageBoxIcon.Error);
                }
            }
        }

        private void Serial_OnDataReceived(string data)
        {
            if (txtDebug.InvokeRequired)
                txtDebug.Invoke(() => AppendDebug(data));
            else
                AppendDebug(data);
        }

        private void AppendDebug(string data)
        {
            txtDebug.AppendText(data + Environment.NewLine);
            var lines = txtDebug.Lines;
            if (lines.Length > 20)
                txtDebug.Lines = lines[^20..];
            txtDebug.SelectionStart = txtDebug.Text.Length;
            txtDebug.ScrollToCaret();
        }

        private void btnRefresh_Click(object sender, EventArgs e)
        {
            RefreshComPorts();
        }

        private void Timer_Tick(object? sender, EventArgs e)
        {
            UpdateTemperatures();
        }

        private void UpdateTemperatures()
        {
            float? cpu = _monitor.GetCpuTemperature();
            float? gpu = _monitor.GetGpuTemperature();
            float? cpuu = _monitor.GetCpuUsage();
            float? cpuc = _monitor.GetCpuClock();
            float? ramu = _monitor.GetRamUsed();
            float? ramav = _monitor.GetRamAvailable();
            float? gpuc = _monitor.GetGpuClock();
            float? vram = _monitor.GetVramUsed();
            float? vramm = _monitor.GetVramTotal();
            float? ghot = _monitor.GetGpuHotspot();

            float ramTotal = (ramu ?? 0f) + (ramav ?? 0f);

            lblCpuTemp.Text =
                $"CPU: {cpu:F1}C  |  " +
                $"Usage: {cpuu:F0}%  |  " +
                $"Clock: {cpuc:F0}MHz  |  " +
                $"RAM: {ramu:F1}/{ramTotal:F1}GB";

            lblGpuTemp.Text =
                $"GPU: {gpu:F1}C  |  " +
                $"Clock: {gpuc:F0}MHz  |  " +
                $"VRAM: {vram:F0}/{vramm:F0}MB  |  " +
                $"Hotspot: {ghot:F1}C";

            _serial.CpuTemp = cpu;
            _serial.GpuTemp = gpu;
            _serial.CpuUsage = cpuu;
            _serial.CpuClock = cpuc;
            _serial.RamUsed = ramu;
            _serial.RamTotal = ramTotal;
            _serial.GpuClock = gpuc;
            _serial.VramUsed = vram;
            _serial.VramTotal = vramm;
            _serial.GpuHotspot = ghot;
        }

        protected override void OnFormClosed(FormClosedEventArgs e)
        {
            _timer.Stop();
            _serial.OnDataReceived -= Serial_OnDataReceived;
            _serial.Disconnect();
            _monitor.Dispose();
            base.OnFormClosed(e);
        }
    }
}