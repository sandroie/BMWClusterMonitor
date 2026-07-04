namespace BMWClusterMonitor
{
    partial class Form1
    {
        private System.ComponentModel.IContainer components = null;
        private System.Windows.Forms.Label lblCpuTemp;
        private System.Windows.Forms.Label lblGpuTemp;
        private System.Windows.Forms.Label lblPortLabel;
        private System.Windows.Forms.Label lblStatus;
        private System.Windows.Forms.ComboBox cmbPorts;
        private System.Windows.Forms.Button btnConnect;
        private System.Windows.Forms.Button btnRefresh;
        private System.Windows.Forms.TextBox txtDebug;
        private System.Windows.Forms.NotifyIcon trayIcon;
        private System.Windows.Forms.ContextMenuStrip trayMenu;

        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
                components.Dispose();
            base.Dispose(disposing);
        }

        private void InitializeComponent()
        {
            lblCpuTemp = new Label();
            lblGpuTemp = new Label();
            lblPortLabel = new Label();
            lblStatus = new Label();
            cmbPorts = new ComboBox();
            btnConnect = new Button();
            btnRefresh = new Button();
            txtDebug = new TextBox();
            SuspendLayout();
            // 
            // lblCpuTemp
            // 
            lblCpuTemp.Font = new Font("Consolas", 10F);
            lblCpuTemp.Location = new Point(20, 20);
            lblCpuTemp.Name = "lblCpuTemp";
            lblCpuTemp.Size = new Size(860, 30);
            lblCpuTemp.TabIndex = 0;
            lblCpuTemp.Text = "CPU: loading...";
            // 
            // lblGpuTemp
            // 
            lblGpuTemp.Font = new Font("Consolas", 10F);
            lblGpuTemp.Location = new Point(20, 55);
            lblGpuTemp.Name = "lblGpuTemp";
            lblGpuTemp.Size = new Size(868, 30);
            lblGpuTemp.TabIndex = 1;
            lblGpuTemp.Text = "GPU: loading...";
            // 
            // lblPortLabel
            // 
            lblPortLabel.AutoSize = true;
            lblPortLabel.Font = new Font("Segoe UI", 9F, FontStyle.Bold);
            lblPortLabel.Location = new Point(20, 105);
            lblPortLabel.Name = "lblPortLabel";
            lblPortLabel.Size = new Size(111, 15);
            lblPortLabel.TabIndex = 2;
            lblPortLabel.Text = "Arduino COM Port:";
            // 
            // lblStatus
            // 
            lblStatus.AutoSize = true;
            lblStatus.Font = new Font("Segoe UI", 9F);
            lblStatus.ForeColor = Color.Red;
            lblStatus.Location = new Point(20, 216);
            lblStatus.Name = "lblStatus";
            lblStatus.Size = new Size(117, 15);
            lblStatus.TabIndex = 6;
            lblStatus.Text = "Status: Disconnected";
            // 
            // cmbPorts
            // 
            cmbPorts.DropDownStyle = ComboBoxStyle.DropDownList;
            cmbPorts.Font = new Font("Segoe UI", 10F);
            cmbPorts.Location = new Point(20, 129);
            cmbPorts.Name = "cmbPorts";
            cmbPorts.Size = new Size(140, 25);
            cmbPorts.TabIndex = 3;
            // 
            // btnConnect
            // 
            btnConnect.Font = new Font("Segoe UI", 10F, FontStyle.Bold);
            btnConnect.Location = new Point(20, 161);
            btnConnect.Name = "btnConnect";
            btnConnect.Size = new Size(228, 36);
            btnConnect.TabIndex = 5;
            btnConnect.Text = "Connect";
            btnConnect.Click += btnConnect_Click;
            // 
            // btnRefresh
            // 
            btnRefresh.Font = new Font("Segoe UI", 9F);
            btnRefresh.Location = new Point(168, 127);
            btnRefresh.Name = "btnRefresh";
            btnRefresh.Size = new Size(80, 28);
            btnRefresh.TabIndex = 4;
            btnRefresh.Text = "Refresh";
            btnRefresh.Click += btnRefresh_Click;
            // 
            // txtDebug
            // 
            txtDebug.BackColor = Color.Black;
            txtDebug.Font = new Font("Consolas", 9F);
            txtDebug.ForeColor = Color.Lime;
            txtDebug.Location = new Point(20, 245);
            txtDebug.Multiline = true;
            txtDebug.Name = "txtDebug";
            txtDebug.ReadOnly = true;
            txtDebug.ScrollBars = ScrollBars.Vertical;
            txtDebug.Size = new Size(860, 200);
            txtDebug.TabIndex = 7;
            // 
            // Form1
            // 
            ClientSize = new Size(917, 457);
            Controls.Add(lblCpuTemp);
            Controls.Add(lblGpuTemp);
            Controls.Add(lblPortLabel);
            Controls.Add(cmbPorts);
            Controls.Add(btnRefresh);
            Controls.Add(btnConnect);
            Controls.Add(lblStatus);
            Controls.Add(txtDebug);
            Name = "Form1";
            Text = "BMW Cluster Monitor";
            trayMenu = new System.Windows.Forms.ContextMenuStrip();
            trayMenu.Items.Add("Show", null, (s, e) => { Show(); WindowState = FormWindowState.Normal; });
            trayMenu.Items.Add("Exit", null, (s, e) => { Application.Exit(); });

            trayIcon = new System.Windows.Forms.NotifyIcon();
            trayIcon.Text = "BMW Cluster Monitor";
            trayIcon.Icon = System.Drawing.SystemIcons.Application; // placeholder icon
            trayIcon.ContextMenuStrip = trayMenu;
            trayIcon.Visible = true;
            trayIcon.DoubleClick += (s, e) => { Show(); WindowState = FormWindowState.Normal; };
            ResumeLayout(false);
            PerformLayout();
        }
    }
}