Quick Start User's Guide for MCS-DR
====================================

    Version: DROS-2.4.1.0-RELEASE
    Date:    February 2026
    Authors: Christopher Wolfe, Jayce Dowell

[![Test](https://github.com/lwa-project/data_recorder/actions/workflows/main.yml/badge.svg)](https://github.com/lwa-project/data_recorder/actions/workflows/main.yml)

[![LWAMemo](https://img.shields.io/badge/lwa%20memo-165-blue)](https://leo.phys.unm.edu/~lwa/memos/memo/lwa0165.pdf)
[![DR_ICD](https://img.shields.io/badge/DR%20ICD-v2.4.1-blue)](Documentation/DR_ICD/)

Version
-------
This quick start applies to DROS-2.4.1.0-RELEASE. For additional
information about versions and features, see `ExtendedUsersGuide.md` and the
DR ICD v2.4.1 in `Documentation/DR_ICD/`.

Prerequisites
-------------
 1. Linux server with x86_64 architecture (e.g., Ubuntu 22.04 or similar)
 2. The following packages: `build-essential`, `lm-sensors`, `smartmontools`,
    `mdadm`, `libfftw3-dev`, `libboost-all-dev`
 3. GNU C/C++ compiler with C++11 support
 4. `systemd` for service management
 5. 10 GbE or faster NIC for data capture


Files and Organization
----------------------
| Directory/File | Description |
|---|---|
| `DROS2/` | Main DROS source code |
| `Msender/` | Command-line messaging utility |
| `DataSource2/` | Simulated data source for testing |
| `SpectrogramViewer/` | Spectrogram viewer |
| `build_helper.py` | Multi-DR build helper |
| `dros.service` | systemd service template |
| `defaults_v2.cfg.example` | Configuration template |
| `Documentation/DR_ICD/` | Interface Control Document (v2.4.1) |

For more details, see `ExtendedUsersGuide.md`.


Step 0: Root Access
-------------------
You must be root for most of the tasks outlined here. The DROS software needs
root permission to work directly with partitions, so `sudo su` before doing
anything is easiest. Building the software can be done in user space, but
installation requires root privileges.

At the bash prompt, type:

    sudo su

and enter the appropriate password for your site.

For any additional terminals you open, repeat this step.


Step 1: Installation
--------------------
> **Note:** Building the `install` target will overwrite files in the `/LWA`
> folder. Existing configuration is automatically backed up and restored.

**Single-DR build:**

```bash
git clone <repo-url>
cd data_recorder
python3 build_helper.py
make -j all
sudo make install
```

**Multi-DR build:**

```bash
python3 build_helper.py 1 2 3
cd build_DR1 && make -j all && sudo make install && cd ..
cd build_DR2 && make -j all && sudo make install && cd ..
cd build_DR3 && make -j all && sudo make install && cd ..
```

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


Step 2: System Configuration
----------------------------
 1. Identify which network interface is the 10 GbE (or faster) data adapter
    and which is the 1 GbE messaging adapter:

    ```bash
    ip link show
    ethtool <interface-name>
    ```

 2. Configure network interfaces according to your site's network environment
    using standard Linux networking tools (e.g., Netplan, NetworkManager).

 3. Copy and edit the DROS configuration:

    ```bash
    cp /LWA/config/defaults_v2.cfg.example /LWA/config/defaults_v2.cfg
    nano /LWA/config/defaults_v2.cfg
    ```

    Key parameters: `MyReferenceDesignator`, `SelfIp`, `MessageInPort`,
    `MessageOutPort`, `DataInIp`, `DataInPort`, `TimeAuthority`, `SerialNumber`,
    `ArraySelect`. See ICD Section 6.3 for full documentation.


Step 3: Storage Preparation
---------------------------
**Single-drive setup (preferred):**

```bash
sudo /LWA/scripts/StorageControl.sh format /dev/sdX MyLabel
```

DROS handles mounting/unmounting automatically. The mount point will be
`/LWA_STORAGE/Internal/{N}/`.

**RAID setup (for legacy DRSU):**

 1. Create the array (substitute your drive letters):

    ```bash
    sudo mdadm -C /dev/md0 -c 256 -l 0 -n 5 /dev/sd{b,c,d,e,f}
    ```

 2. Persist configuration:

    ```bash
    sudo cp /etc/mdadm/mdadm.conf /etc/mdadm/mdadm.conf.bak
    sudo mdadm --examine --scan > /etc/mdadm/mdadm.conf
    ```

 3. Format for DROS:

    ```bash
    sudo /LWA/scripts/StorageControl.sh format /dev/md0 MyDrsuBarcode
    ```

See `ExtendedUsersGuide.md` for more details on storage setup.


Step 4: First Run
-----------------
For the first run, launch DROS directly to observe startup output:

```bash
sudo /LWA/bin/DROS2
```

> **Note:** The `binary-mode` file in `/LWA/config/` controls whether DROS
> runs in Spectrometer or LiveBuffer mode. It defaults to Spectrometer.

You should see output ending with:

```
[I] [System          ] [System] Booting Complete
```

The system is now up and listening for command messages.


Step 5: Test Messaging
----------------------
At this point, the system is up and listening for command messages.
To test messaging, use the included `Msender` utility.

In a new terminal:

```bash
/LWA/bin/Msender -v -Source "MCS" -Destination "DR1" -Type "PNG" \
    -ReferenceNumber 0 -DestinationIpAddress localhost \
    -DestinationPort 5001 -ResponseListenPort 5000
```

You should see an accepted response with `General Status: NORMAL`.


Step 6: Exit First Run
----------------------
In the new terminal from step 5:

```bash
/LWA/bin/Msender -v -Source "MCS" -Destination "DR1" -Type "SHT" \
    -ReferenceNumber 0 -DestinationIpAddress localhost \
    -DestinationPort 5001 -ResponseListenPort 5000
```

The DROS terminal should show `[System] Shutdown Complete` and the program
will exit.


Step 7: Normal Operation Mode
-----------------------------
Install the DROS service for automatic startup on boot:

```bash
sudo systemctl daemon-reload
sudo systemctl enable dros-dr1.service
sudo systemctl start dros-dr1.service
```

For multi-DR, enable each service (e.g., `dros-dr1`, `dros-dr2`, etc.).

Reboot and verify DROS is responding to messaging as in Step 5.

Service logs can be viewed with:

```bash
journalctl -u dros-dr1
```

For service management commands (start/stop/restart/status), see
`ExtendedUsersGuide.md`.


Step 8: Scheduling a Recording
------------------------------
The format of scheduling a recording is:

```bash
/LWA/bin/Msender -s MCS -d DR1 -r 1 -I 127.0.0.1 -po 5001 -pi 5000 -v \
    -t "REC" -D "<MJD> <MPM> <duration_ms> <FORMAT>"
```

Select appropriate values for MJD, MPM, duration, and FORMAT. Available
formats include `DEFAULT_DRX`, `DEFAULT_DRX8`, `DEFAULT_TBT`, `DEFAULT_TBS`,
`DEFAULT_COR`, and filter-specific variants. See ICD Appendix C for the full
list.

Example (10-second DRX recording):

```bash
/LWA/bin/Msender -s MCS -d DR1 -r 1 -I 127.0.0.1 -po 5001 -pi 5000 -v \
    -t "REC" -D "060791 13500000 10000 DEFAULT_DRX"
```

The response will include the filename assigned to the recording.

To verify the recording, query the directory:

```bash
/LWA/bin/Msender -s MCS -d DR1 -r 2 -I 127.0.0.1 -po 5001 -pi 5000 -v \
    -t "RPT" -D "DIRECTORY-ENTRY-1"
```


Step 8: Retrieving Recorded Data
---------------------------------
The GET command retrieves small chunks of data (~8000 bytes max per request):

```bash
/LWA/bin/Msender -s MCS -d DR1 -r 3 -I 127.0.0.1 -po 5001 -pi 5000 -v \
    -t "GET" -D "060791_000000001 000000000 4096"
```

This retrieves the first 4096 bytes from the specified recording file.
Recording filenames are constructed from the MJD and reference number of the
REC command.

Additional retrieval methods (CPY, DMP) and live wire testing are covered in
`ExtendedUsersGuide.md`.


Additional Information
----------------------
- **Extended User's Guide:** `ExtendedUsersGuide.md` — detailed setup,
  operation, and troubleshooting
- **DR ICD v2.4.1:** `Documentation/DR_ICD/` — authoritative reference for
  commands, MIB entries, data formats, and specifications
- **Issues:** [GitHub Issues](https://github.com/lwa-project/data_recorder/issues)
