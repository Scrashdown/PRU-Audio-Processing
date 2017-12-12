# Usage of resources depending on the CIC filter parameters `N`, `M`, `R` and on the number of channels `C`

In the current design, we use only registers to store all values involved in the CIC filter computation. Each PRU has 32 registers `(r0-r31)`, but in practice only 30 are usable. This is because `r30` and `r31` are reserved for interacting with GPIO and triggering interrupts.

Furthermore, since we use the PRU's additional register banks, we cannot use all the bits of `r0`, because the value of the first byte of `r0` is used as an offset value for register exchanges with the banks.

Another thing we need to take into account is that, apart from channel private data (values involved in the CIC filter computations for each channel), we also need to keep track of some channel _independent_ data : a byte counter to keep track of the number of bytes written in the host buffer, a sample counter to implement decimation, 2 registers to store the host buffer's size and address and a temporary register to store temporary values used for delaying and processing input.

The remaining registers can be used for storing channel data. In order to figure out how many registers we need, we can use the following formula `(M = 1)` :

	n_reg = C * (2N + 1 + N - 1) = 3CN

With `M != 1`:

	// TODO

Where `C` is the number of channels, `N` is the number of integrator and comb stages in the filter (that is, `N` integrators and `N` combs, `M` is the number of samples per stage of the filter (usually `M = 1`) and `R` is the decimation rate. If the filter's input is at frequency `f_s`, its output will be at frequency `f_s / R`.
It is therefore obvious that `R` has no influence on the filter's register usage. However, `R` influences the filter's output data rate.

In our case, `C = 6`, `M = 1`, `N = 4` and `R = 16`, so `n_reg = 72`. However, this is assuming we use 1 output register per channel and write them all at once to the buffer, which requires 6 registers in this case. To spare registers, we use 3 registers for 6 channels and write the outputs to the host in 2 steps, so we can reduce `n_reg` to 69. Using this strategy, we can modify our formula the following way :

	n_reg = 3CN - 3 = 3*(CN - 1)

We also want to figure out the data rate of the fiter's output. To do this, we need to compute the output's bit width using the following formula described in Hogenhauer's paper :

	B_out = ceil(Nlog2(RM) + B_in)

Where `B_in` is the input bit width. In our case, `B_in = 1`, so `B_out = 25`. However, the PRU's registers contain 32 bits and it is easier to write the data in chunks of which the size is a multiple of 8. Therefore, our 'effective' output bit width, `B_out'` is 32. Since we know the output sample rate is `f_s / R`, it is now straightforward to compute the output data rate :

	D_out = B_out * f_s / R

Or using the `B_out'` :

	D_out' = B_out' * f_s / R

In our case, `f_s ~= 1.02 MHz`, `R = 16` and `B_out' = 32`, which gives `D_out' = 2.04 Mb/s = 255 kB/s`.

## Number of registers required given `N` and `C` (`M = 1`)

|       | N | 1 | 2 | 3 | 4  | 5 |
|---    |---|---|---|---|--- |---|
| **C** |   |   |   |   |    |   |
| **1** |   |   |   |   | 11 |   |
| **2** |   |   |   |   |    |   |
| **3** |   |   |   |   |    |   |
| **6** |   |   |   |   |    |   |
| **8** |   |   |   |   |    |   |

## Output data rate for 1 channel, given `N` and `R` (`M = 1`)

|        |  N  |  1  |  2  |  3  |  4  |  5  |
|---     |:---:|:---:|:---:|:---:|:---:|:---:|
| **R**  |     |     |     |     |     |     |
| **16** |     |     |     |     |     |     |
| **32** |     |     |     |     |     |     |
| **48** |     |     |     |     |     |     |
| **64** |     |     |     |     |     |     |