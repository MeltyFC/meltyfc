# MeltyFC — Clock/Electrical Spec Compliance Audit
## Re-run this table on ANY clock_config.c change (I-20)

| Target | Param | Configured | RM Limit | Verdict |
|--------|-------|-----------|----------|---------|
| crux F405 | VCO/SYSCLK | 336/168 | 100-432/168 @VOS1 | OK |
| | USB (PLLQ=7) | 48.000 | 48 +/-0.25% | OK exact |
| | AHB/APB1/APB2 | 168/42/84 | 168/42/84 | OK at-cap |
| | Flash latency | 5WS | 5WS @150-168 | OK |
| betafpv F411 | SYSCLK | 96 | <=100 @Scale1 | OK (CS-1) |
| | USB (PLLQ=8) | 48.000 | 48 +/-0.25% | OK exact (CS-1) |
| | Flash/buses | 3WS/96/48/96 | 3WS/100/50/100 | OK |
| aikon F722 | SYSCLK/OD | 216/enabled | 216 needs Scale1+OD | OK |
| | USB (PLLQ=9) | 48.000 | exact | OK |
| | Latency/buses | 7WS/54/108 | 7WS/54/108 | OK |
| jhemcu F745 | (identical to F722) | | | OK |
| micoair H743 | VCO/SYSCLK | 800/400 | 192-836/400 @VOS1 | OK at-cap |
| | HCLK/APBs/latency | 200/100/2WS | 240/120/2WS | OK |
| | USB | HSI48+CRS | 48MHz | OK (CS-2) |
| betafpv H725 | VCO/SYSCLK/VOS | 800/400/VOS1 | 192-836/400 @VOS1 | OK (CS-3) |
| | buses/latency | 200/100/2WS | 275/137.5/2WS | OK |
| | USB | HSI48+CRS | 48MHz | OK (CS-2) |

All PLL input ranges, RGE settings, and bus prescaler caps verified.
All I-16 timer-clock constants independently re-derived per family.
