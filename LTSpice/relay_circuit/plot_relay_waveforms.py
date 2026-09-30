#!/usr/bin/env python3
import struct
import numpy as np
import matplotlib.pyplot as plt

def parse_raw(filepath):
    with open(filepath, 'rb') as f:
        raw = f.read()

    hdr_marker = b'B\x00i\x00n\x00a\x00r\x00y\x00:\x00\n\x00'
    hdr_idx = raw.find(hdr_marker)
    if hdr_idx != -1:
        header = raw[:hdr_idx].decode('utf-16le')
        body = raw[hdr_idx + len(hdr_marker):]
    else:
        hdr_marker = b'Binary:\n'
        hdr_idx = raw.find(hdr_marker)
        header = raw[:hdr_idx].decode('utf-8', errors='ignore')
        body = raw[hdr_idx + len(hdr_marker):]

    lines = [l.strip() for l in header.splitlines() if l.strip()]
    n_vars = int([l.split(':')[1].strip() for l in lines if 'No. Variables:' in l][0])
    n_pts = int([l.split(':')[1].strip() for l in lines if 'No. Points:' in l][0])
    
    var_idx_start = [i for i, l in enumerate(lines) if l.startswith('Variables:')][0] + 1
    var_lines = lines[var_idx_start : var_idx_start + n_vars]
    var_names = [v.split()[1] for v in var_lines]

    pt_size = 8 + (n_vars - 1) * 4
    time_arr = np.zeros(n_pts)
    data = {name: np.zeros(n_pts) for name in var_names[1:]}

    for i in range(n_pts):
        offset = i * pt_size
        t = struct.unpack('<d', body[offset : offset + 8])[0]
        time_arr[i] = abs(t)
        vals = struct.unpack(f'<{n_vars-1}f', body[offset + 8 : offset + pt_size])
        for idx, name in enumerate(var_names[1:]):
            data[name][i] = vals[idx]

    return time_arr, data

def main():
    t, data = parse_raw('LTSpice/relay_circuit/relay_circuit.raw')
    t_ms = t * 1e3

    fig, axes = plt.subplots(4, 1, figsize=(12, 10), sharex=True)
    plt.subplots_adjust(hspace=0.28)

    # 1. MCU Logic Controls
    ax1 = axes[0]
    ax1.plot(t_ms, data['V(mcu_relay_pin)'], label='RELAY_EN (MCU Pin)', color='#1f77b4', lw=2)
    ax1.plot(t_ms, data['V(pol_driver_out)'], label='POL_EN (MCU Pin)', color='#ff7f0e', lw=1.8, linestyle='--')
    ax1.set_ylabel('MCU Logic (V)', fontweight='bold')
    ax1.set_ylim(-0.5, 6.0)
    ax1.grid(True, alpha=0.3)
    ax1.legend(loc='upper right')
    ax1.set_title('1. Microcontroller Timing: Cold-Switching Relay Sequence', fontweight='bold', fontsize=12)

    # 2. Relay Coil Drive & Flyback Protection
    ax2 = axes[1]
    ax2.plot(t_ms, data['V(v_actuate)'], label='Relay Coil Voltage (V_vcc - V_drain)', color='#9467bd', lw=2)
    ax2.axhline(3.0, color='gray', linestyle=':', label='NO Contact Threshold (3.0 V)')
    ax2.axhline(2.0, color='gray', linestyle='--', label='NC Contact Threshold (2.0 V)')
    ax2.set_ylabel('Coil Drive (V)', fontweight='bold')
    ax2.set_ylim(-0.5, 6.0)
    ax2.grid(True, alpha=0.3)
    ax2.legend(loc='upper right')
    ax2.set_title('2. Relay Coil Actuation & Break-Before-Make Thresholds', fontweight='bold', fontsize=12)

    # 3. Sensor Coil Current & TVS Quench Voltage
    ax3 = axes[2]
    ax3_twin = ax3.twinx()
    p1 = ax3.plot(t_ms, data['I(Lcoil)'], label='Coil Current I(LCOIL)', color='#2ca02c', lw=2)
    p2 = ax3_twin.plot(t_ms, data['V(ccs_minus)'], label='MOSFET Drain / TVS Clamp', color='#d62728', lw=1.5, alpha=0.85)
    ax3.set_ylabel('Coil Current (A)', color='#2ca02c', fontweight='bold')
    ax3_twin.set_ylabel('TVS / Drain (V)', color='#d62728', fontweight='bold')
    ax3.set_ylim(-0.2, 2.4)
    ax3_twin.set_ylim(-5, 60)
    ax3.grid(True, alpha=0.3)
    lines = p1 + p2
    labels = [l.get_label() for l in lines]
    ax3.legend(lines, labels, loc='upper right')
    ax3.set_title('3. Sensor Coil Current (~1.98 A) & Fast TVS Quench (51.3 V Clamping)', fontweight='bold', fontsize=12)

    # 4. Readout Output Port Voltage
    ax4 = axes[3]
    v_diff = (data['V(readout_pos)'] - data['V(readout_neg)']) * 1e3
    ax4.plot(t_ms, v_diff, label='V(READOUT+ - READOUT-)', color='#008080', lw=1.8)
    ax4.set_ylabel('Readout (mV)', fontweight='bold')
    ax4.set_xlabel('Time (ms)', fontweight='bold')
    ax4.grid(True, alpha=0.3)
    ax4.legend(loc='upper right')
    ax4.set_title('4. Readout Port: Zero Hot-Switching Spikes, Clean Transfer to Preamp', fontweight='bold', fontsize=12)

    plt.suptitle('Proton Magnetometer DPDT Relay & Cold-Switching Simulation', fontsize=14, fontweight='bold', y=0.99)
    plt.savefig('LTSpice/relay_circuit/relay_circuit_waveforms.png', dpi=200, bbox_inches='tight')
    print('Saved LTSpice/relay_circuit/relay_circuit_waveforms.png')

if __name__ == '__main__':
    main()
