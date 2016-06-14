using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Runtime.InteropServices.WindowsRuntime;
using System.Threading.Tasks;
using Windows.Devices.Enumeration;
using Windows.Devices.Midi;
using Windows.Devices.Sms;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.System.Threading;
using Windows.UI.Notifications;
using Windows.UI.Popups;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Documents;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;
using MIDI;
using Win32;

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

namespace Clockwork
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MainPage : Page
    {

        /// <summary>
        /// Collection of active MidiOutPorts
        /// </summary>
        private List<IMidiOutPort> midiOutPorts;

        /// <summary>
        /// Device watcher for MIDI out ports
        /// </summary>
        MidiDeviceWatcher midiOutDeviceWatcher;

        /// <summary>
        /// Keep track of the current output device (which could also be the GS synth)
        /// </summary>
        IMidiOutPort currentMidiOutputDevice;

        long delay = 50000;

        private Action messaging;
        public MainPage()
        {
            this.InitializeComponent();
            
            this.midiOutPorts = new List<IMidiOutPort>();

            // Set up the MIDI output device watcher
            this.midiOutDeviceWatcher = new MidiDeviceWatcher(MidiOutPort.GetDeviceSelector(), Dispatcher, this.outputDevices);

            // Start watching for devices
            this.midiOutDeviceWatcher.Start();

            messaging = SendMessage;
        }

        private void MainPage_OnLoaded(object sender, RoutedEventArgs e)
        {
            
        }

        private async void outputDevices_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            // Get the selected output MIDI device
            int selectedOutputDeviceIndex = this.outputDevices.SelectedIndex;

            // Try to create a MidiOutPort
            if (selectedOutputDeviceIndex < 0)
            {
                return;
            }

            DeviceInformationCollection devInfoCollection = this.midiOutDeviceWatcher.GetDeviceInformationCollection();
            if (devInfoCollection == null)
            {
                return;
            }

            DeviceInformation devInfo = devInfoCollection[selectedOutputDeviceIndex];
            if (devInfo == null)
            {
                return;
            }

            this.currentMidiOutputDevice = await MidiOutPort.FromIdAsync(devInfo.Id);
            if (this.currentMidiOutputDevice == null)
            {
                return;
            }

            // We have successfully created a MidiOutPort; add the device to the list of active devices
            if (!this.midiOutPorts.Contains(this.currentMidiOutputDevice))
            {
                this.midiOutPorts.Add(this.currentMidiOutputDevice);
            }
        }

        private async void button_Click(object sender, RoutedEventArgs e)
        {
            this.currentMidiOutputDevice?.SendMessage(new MidiStartMessage());
            await ThreadPool
                .RunAsync(_ =>
                {
                    var watch = Stopwatch.StartNew();

                    while (true)
                    {
                        if (watch.ElapsedTicks >= delay)
                        {
                            Task.Run(messaging);
                            watch.Restart();
                        }
                    }
                }, WorkItemPriority.High);

        }

        private void SendMessage()
        {
            this.currentMidiOutputDevice?.SendMessage(new MidiTimingClockMessage());
        }

        private void ticks_TextChanged(object sender, TextChangedEventArgs e)
        {
            delay = long.Parse(ticks.Text);
        }
    }
}
