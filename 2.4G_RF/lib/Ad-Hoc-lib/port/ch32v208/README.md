# CH32V208 Port

This directory contains CH32V208-specific port glue for `Ad-Hoc-lib`.

Current content:

- `adhoc_port_ch32_now_tc/now_us`: wraps AROS tick source into protocol time base.
- `adhoc_port_ch32_rand_u16`: uses TMOS standard-library RNG (`rand/srand`) as platform random source.
- `adhoc_port_ch32_critical_enter/exit`: wraps interrupt-level critical section hooks.
