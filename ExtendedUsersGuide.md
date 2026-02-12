# Extended User's Guide for MCS-DR

    Version: DROS-2.4.1.0-RELEASE
    Date:    February 2026
    Authors: Christopher Wolfe, Jayce Dowell

## Version History

| DROS Version | Milestone |
|---|---|
| 0.8 | Initial release |
| 1.0 | Filesystem read/write, MIB branch 1 |
| 1.3 | Added SPC command (spectrometer) |
| 1.5.0 | Beam-correlator, saturation counts, DRSU barcode |
| 2.0 | Stokes IV/IQUV, flexible spectrometer, metatags |
| 2.4.1 | NDP support, DRX8/TBT/TBS/COR formats, multi-DR, systemd. See DR ICD v2.4.1 for full details. |


## Prerequisites

This guide assumes the following:

1. A Linux server with x86_64 architecture (e.g., Ubuntu 22.04 or similar)
2. The following packages are installed:
   - `lm-sensors`, `smartmontools`, `mdadm`
   - `libfftw3-dev`, `libboost-all-dev`
   - GNU C/C++ compiler with C++11 support (`build-essential`)
3. `systemd` for service management
4. A 10 GbE or faster NIC for data capture
5. Familiarity with Linux (build is `make all install`, no configure step)
6. Optional: another PC or comparable machine to act as an emulated data source
   for testing in lieu of NDP
7. Root privileges are required for installation and most runtime operations.
   Building the software can be done in user space.


## Files and Organization

The source repository contains the following key components:

| Directory/File | Description |
|---|---|
| `DROS2/` | Main DROS source code |
| `Msender/` | Command-line utility to send/receive MCS messages |
| `DataSource2/` | Simulated data source for testing |
| `SpectrogramViewer/` | GUI utility to view spectrograms |
| `build_helper.py` | Multi-DR build helper script |
| `dros.service` | systemd service template |
| `defaults_v2.cfg.example` | Configuration template |
| `Documentation/DR_ICD/` | Interface Control Document (v2.4.1) |


## Software Installation

### Single-DR Build

Clone the repository and build:

```bash
git clone <repo-url>
cd data_recorder
python3 build_helper.py
cd DROS2/Debug && make -j all && cd ../..
make -j all
sudo make install
```

> **Note:** `INSTALL_LOCATION` (default `/LWA`) and `STORAGE_LOCATION`
> (default `/LWA_STORAGE`) can be overridden on the `make install` command
> line, e.g., `make install INSTALL_LOCATION=/opt/LWA`.

Building the `install` target will overwrite all files in the install
location, but existing configuration files (`defaults_v2.cfg`) are
automatically backed up and restored.

This should generate output ending with:

```
################################################################
# Notice:
#
# MCS-DR software has been installed to:
#       /LWA
#
# The default configuration files are installed to:
#       /LWA/config/
#
# You must modify defaults_v2.cfg.example to reflect your network environment.
#
# To launch the software, execute:
#       /LWA/bin/DROS2
#
# To install the software to run on-boot, execute:
#       systemctl daemon-reload
#       systemctl enable dros-dr*.service
#
################################################################
```

### Multi-DR Build

For systems running multiple DR instances, use `build_helper.py` with
numeric DR IDs:

```bash
python3 build_helper.py 1 2 3
```

This creates `build_DR1/`, `build_DR2/`, `build_DR3/`, etc. Each has its
own Makefile with `INSTALL_LOCATION` set to `/LWA/DR{N}` and
`STORAGE_LOCATION` set to `/LWA_STORAGE/DR{N}`. Build and install each
separately:

```bash
cd build_DR1 && make -j all && sudo make install && cd ..
cd build_DR2 && make -j all && sudo make install && cd ..
cd build_DR3 && make -j all && sudo make install && cd ..
```

The `build_helper.py` script also accepts `-n/--n-stand` to set the number
of stands for TBT/TBS modes (default: 256).  This option is needed for
mini-station deployments so that DROS can calculate the correct packet sizes for
TBS and TBT data.


## System Configuration

Before DROS can be used, configuration files must be edited to reflect your
local network environment. The configuration files are located in the
config directory (e.g., `/LWA/config/` for single-DR or `/LWA/DR1/config/`
for multi-DR).

### Network Identification

The DR server will have at least two Ethernet adapters: one 10 GbE (or faster)
for NDP data, and one GbE for messaging. To identify which is which, use:

```bash
ip link show
ethtool <interface-name>
```

The high-speed adapter is used for data capture; the other is for MCS
messaging. Configure these interfaces according to your site's network
environment using standard Linux networking tools (e.g., Netplan, NetworkManager,
or `/etc/network/interfaces`).

### DROS Configuration

Copy the example configuration and edit it:

```bash
cp /LWA/config/defaults_v2.cfg.example /LWA/config/defaults_v2.cfg
nano /LWA/config/defaults_v2.cfg
```

The configuration parameters are:

| Parameter | Description | Default |
|---|---|---|
| `MyReferenceDesignator` | Reference designator (e.g., DR1, DR2) | `DR1` |
| `SelfIp` | DR's IP address | `192.168.1.20` |
| `MessageInPort` | UDP port for incoming MCS messages | `5000` |
| `MessageOutPort` | UDP port for outgoing responses | `5001` |
| `DataInIp` | DR's IP address for NDP data | `192.168.40.41` |
| `DataInPort` | UDP port for NDP data | `16180` |
| `TimeAuthority` | NTP server URL or IP | `Time.Ubuntu.com` |
| `SerialNumber` | System serial number | `0` |
| `ArraySelect` | Startup DRSU selection (barcode or 0-based index) | `1` |

For multi-DR setups, each DR instance has its own config directory
(e.g., `/LWA/DR1/config/`, `/LWA/DR2/config/`).

> See ICD Section 6.3 for full parameter documentation.


## Storage Preparation

### Single-Drive Setup (Preferred)

Format a drive for use with DROS using the included `StorageControl.sh`
script:

```bash
sudo /LWA/scripts/StorageControl.sh format /dev/sdX MyLabel
```

This creates a DROSv2-compliant EXT4 filesystem. The mount point will be
`/LWA_STORAGE/Internal/{N}/` where `{N}` is a 0-based index. DROS handles
mounting and unmounting automatically.

### RAID Setup (Legacy DRSU)

For DRSU storage units with multiple drives, create a software RAID-0
array:

1. Create the array (substitute your actual drive letters):

   ```bash
   sudo mdadm -C /dev/md0 -c 256 -l 0 -n 5 /dev/sd{b,c,d,e,f}
   ```

   If the DRSU was previously configured and you want to preserve data:

   ```bash
   sudo mdadm --assemble /dev/md0 /dev/sdb /dev/sdc /dev/sdd /dev/sde /dev/sdf
   ```

2. Persist the configuration across reboots:

   ```bash
   sudo cp /etc/mdadm/mdadm.conf /etc/mdadm/mdadm.conf.bak
   sudo mdadm --examine --scan > /etc/mdadm/mdadm.conf
   ```

3. Format the array for DROS:

   ```bash
   sudo /LWA/scripts/StorageControl.sh format /dev/md0 MyDrsuBarcode
   ```

DROS will mount the array automatically on startup. To mount manually for
inspection:

```bash
sudo mount -t ext4 -o defaults,data=writeback,noatime,barrier=0 /dev/md0 <mountpoint>
```

> **Note:** If mdadm scans drives before all DRSU disks have initialized,
> partially started arrays may appear with unexpected names (e.g.,
> `/dev/md_d0`). See the Troubleshooting section for resolution.


## First Run

For the first run, launch DROS directly rather than via systemd so you can
observe the startup output. The binary path is `/LWA/bin/DROS2`.

> **Note:** The `binary-mode` file in the config directory controls whether
> DROS runs in Spectrometer or LiveBuffer mode. If not present, it defaults
> to Spectrometer mode.

```bash
sudo /LWA/bin/DROS2
```

You should see output similar to:

```
========================== Logfile rotated ==========================
========================== Logfile opened ==========================
[I] [System          ] [System] Booting Start
[I] [System          ] [System] Hardware CPU count: 8
[I] [System          ] [System] Read config...
[I] [System          ] Configuration data loaded...
[D] [System          ] MyReferenceDesignator         DR1
[D] [System          ] SelfIp                        192.168.1.20
[D] [System          ] MessageInPort                 5000
[D] [System          ] MessageOutPort                5001
[D] [System          ] TimeAuthority                 Time.Ubuntu.com
[D] [System          ] SerialNumber                  0
[D] [System          ] ArraySelect                   1
[D] [System          ] DataInPort                    16180
[I] [System          ] [System] Scan storage devices...
[I] [System          ] Beginning storage subsystem initialization.

< will pause here while storage is scanned >

[I] [System          ] Storage subsystem initialized!!!
    ...
[I] [System          ] [System] Booting Complete
```

> Exact log format and timestamps may vary. The key indicator of success is
> the `[System] Booting Complete` message.

At this point, the system is up and listening for command messages. It will
periodically display buffer usage:

```
[I] [Receiver        ] [Receiver] Buffer usage: [________________________________________] (  0%)
[I] [Receiver        ] [Receiver] Receive rate: [________________________________________] (  0%)  0.00000 B/s
[I] [Receiver        ] [Receiver] Subscribers 0
```

### Test Messaging

To test messaging, use the included `Msender` utility. In a new terminal:

```bash
/LWA/bin/Msender -v -Source "MCS" -Destination "DR1" -Type "PNG" \
    -ReferenceNumber 0 -DestinationIpAddress localhost \
    -DestinationPort 5001 -ResponseListenPort 5000
```

On the DROS terminal you should see the message received and an accepted
response sent back. On the Msender terminal you should see the response
with `Accept/Reject: Accepted` and `General Status: NORMAL`.

### Shutdown Test

To shut down the first run, send a SHT command:

```bash
/LWA/bin/Msender -v -Source "MCS" -Destination "DR1" -Type "SHT" \
    -ReferenceNumber 0 -DestinationIpAddress localhost \
    -DestinationPort 5001 -ResponseListenPort 5000
```

The DROS terminal should show the shutdown sequence ending with:

```
[I] [System          ] [System] Shutdown Complete
========================== Logfile closed ==========================
```


## Normal Operation

Once communication has been verified, install DROS as a systemd service for
automatic startup.

### Single-DR

```bash
sudo systemctl daemon-reload
sudo systemctl enable dros-dr1.service
sudo systemctl start dros-dr1.service
```

### Multi-DR

Enable and start each service individually:

```bash
sudo systemctl daemon-reload
sudo systemctl enable dros-dr1.service dros-dr2.service dros-dr3.service
sudo systemctl start dros-dr1.service dros-dr2.service dros-dr3.service
```

### Service Management

| Action | Command |
|---|---|
| Check status | `systemctl status dros-dr1` |
| Start | `sudo systemctl start dros-dr1` |
| Stop | `sudo systemctl stop dros-dr1` |
| Restart | `sudo systemctl restart dros-dr1` |
| View logs | `journalctl -u dros-dr1` |
| Follow logs | `journalctl -u dros-dr1 -f` |

Reboot the system and verify that DROS is responding to messaging as in
the first-run test above.


## Scheduling a Recording

The REC command schedules a recording. The general format using `Msender`:

```bash
/LWA/bin/Msender -Source MCS -Destination DR1 -Type REC \
    -ReferenceNumber <ref> \
    -DestinationIpAddress localhost -DestinationPort 5001 \
    -ResponseListenPort 5000 \
    -Data "<MJD> <MPM> <duration_ms> <FORMAT>"
```

Where:
- `<MJD>` is the Modified Julian Day of the recording start
- `<MPM>` is the start time in milliseconds past midnight
- `<duration_ms>` is the recording length in milliseconds
- `<FORMAT>` is the data format name (e.g., `DEFAULT_DRX`)

The request must be received at least 5-10 seconds before the scheduled
start time.

### Example

```bash
/LWA/bin/Msender -v -Source MCS -Destination DR1 -Type REC -ReferenceNumber 1 \
    -DestinationIpAddress localhost -DestinationPort 5001 -ResponseListenPort 5000 \
    -Data "060791 13500000 10000 DEFAULT_DRX"
```

Or using the short flags:

```bash
/LWA/bin/Msender -s MCS -d DR1 -r 1 -I 127.0.0.1 -po 5001 -pi 5000 -v \
    -t "REC" -D "060791 13500000 10000 DEFAULT_DRX"
```

The response will include the filename assigned to the recording (e.g.,
`060791_000000001`). In the DROS log, you will see the recording being
scheduled, started, and completed.

> See ICD Section 4.7 (REC command) and Appendix C for the full list of
> supported data formats and their data rates.

### Available Data Formats

| Format | Description |
|---|---|
| `DEFAULT_DRX` | 4-bit DRX, full rate |
| `DEFAULT_DRX8` | 8-bit DRX, full rate |
| `DEFAULT_TBT` | Transient Buffer -- Triggered |
| `DEFAULT_TBS` | Transient Buffer -- Streaming (12 channels, default) |
| `DEFAULT_COR` | Correlator output |
| `DRX_FILT_1` through `DRX_FILT_7` | 4-bit DRX, filter-specific rates |
| `DRX8_FILT_1` through `DRX8_FILT_7` | 8-bit DRX, filter-specific rates |
| `TBS_FILT_7` through `TBS_FILT_9` | TBS with 4, 8, or 12 channels |


## Retrieving Recorded Data

There are three methods of retrieving recorded data:

1. **GET** - Retrieve small chunks of data (up to ~8000 bytes per request)
2. **CPY** - Copy a selection of data to a file on an external drive
3. **DMP** - Copy a selection into a series of files on an external drive

### GET Example

```bash
/LWA/bin/Msender -v -Source MCS -Destination DR1 -Type GET -ReferenceNumber 2 \
    -DestinationIpAddress localhost -DestinationPort 5001 -ResponseListenPort 5000 \
    -Data "060791_000000001 000000000 4096"
```

This retrieves the first 4096 bytes from the file `060791_000000001`.
Recording filenames are constructed automatically from the MJD and
reference number of the REC command.

### CPY Example

```bash
/LWA/bin/Msender -v -Source MCS -Destination DR1 -Type CPY -ReferenceNumber 3 \
    -DestinationIpAddress localhost -DestinationPort 5001 -ResponseListenPort 5000 \
    -Data "060791_000000001 000000000 000004096 /dev/sdk MyDataCopy"
```

### DMP Example

```bash
/LWA/bin/Msender -v -Source MCS -Destination DR1 -Type DMP -ReferenceNumber 4 \
    -DestinationIpAddress localhost -DestinationPort 5001 -ResponseListenPort 5000 \
    -Data "060791_000000001 000000000 000004096 000000016 /dev/sdk MyDataSeries"
```

CPY and DMP are scheduled operations. The `Msender` response will return
immediately, but the operation takes time to complete. Monitor progress
via the DROS log or by querying the `OP-TYPE` MIB entry.

> See the ICD for full command specifications and argument formats.


## Live Wire Test (Recording Real Data)

To test recording with actual data, use the included `DataSource2`
emulator.

### DRX Example

In a separate terminal (or via SSH from another machine with DataSource2
installed):

```bash
/LWA/bin/DataSource2 -i localhost -p 16180 -DRX -Duration 30000
```

### DRX8 Example

```bash
/LWA/bin/DataSource2 -i localhost -p 16180 -DRX8 -Duration 30000
```

### Available DataSource2 Flags

| Flag | Format |
|---|---|
| `-DRX` | 4-bit DRX data |
| `-DRX8` | 8-bit DRX data |
| `-TBT` | TBT data |
| `-TBS` | TBS data |
| `-COR` | Correlator data |
| `-RAW` | Raw data |

Schedule a recording as described in the previous section, ensuring that
the data source is running before the recording starts and continues past
the recording end. The receive rate in the DROS log should be non-zero
when data is being captured.


## Troubleshooting

### General

The first thing to check if things aren't working is the DROS log output.
Use `journalctl -u dros-dr1` to view the service log, or check the console
output if running manually.

If DROS is responding to messages, using `Msender` to read MIB entries can
also be helpful for diagnosing issues.

### Startup Termination

If the initial run terminates, it is generally because some configuration
has not been completed. The log output will indicate where the error
occurred. Typically this will be due to an issue with internal storage or
drive detection. Verify that storage is properly configured per the Storage
Preparation section.

### Binary Mode Issues

The `binary-mode` file in the config directory (`/LWA/config/binary-mode`)
controls whether DROS runs in `Spectrometer` or `LiveBuffer` mode. If DROS
is not behaving as expected, verify this file contains the correct mode
string.

### systemd Service Issues

```bash
# Check service status and recent errors
systemctl status dros-dr1

# View full log output
journalctl -u dros-dr1 --no-pager

# View log from the last boot
journalctl -u dros-dr1 -b
```

### mdadm / RAID Issues

In a DRSU environment, if the `md` kernel module or mdadm scans
automatically before all drives have initialized, partially started arrays
may appear with unexpected names (e.g., `/dev/md_d0`).

To check the current RAID state:

```bash
cat /proc/mdstat
```

A healthy array looks like:

```
md0 : active raid0 sdb[0] sdc[1] sdd[2] sde[3] sdf[4]
      52427520 blocks 256k chunks
```

If the array has the wrong name or is inactive, stop it and reassemble:

```bash
sudo mdadm --stop /dev/md_d0
sudo mdadm --assemble /dev/md0 /dev/sdb /dev/sdc /dev/sdd /dev/sde /dev/sdf
```

Then persist the configuration:

```bash
sudo mdadm --examine --scan > /etc/mdadm/mdadm.conf
```

### Getting Help

Please report bugs or ask questions via
[GitHub Issues](https://github.com/lwa-project/data_recorder/issues).


## ICD Reference

The DR ICD v2.4.1 (`Documentation/DR_ICD/`) is the authoritative reference
for formal interface specifications, including:

- All MCS commands and their formats
- MIB entries and response formats
- Data format specifications and rates
- Spectrometer file format and Stokes products
- Error messages and status codes

This user's guide is intended as a companion operational/tutorial document.
For any discrepancy between this guide and the ICD, the ICD takes
precedence.
