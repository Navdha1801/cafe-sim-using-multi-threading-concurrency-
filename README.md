1. During the simulation:

    - There are K coffee types.
    - Each coffee type c takes some time t_c to prepare.

    - The cafe has B baristas.
    - There are N customers waiting to get their coffee.
    - Each customer i arrives at some time t_arr_i, and orders only one coffee x.
    - Each customer i arrives at some tolerance time tol_i, after which their patience runs out and they will instantly leave the cafe (bad).

2. Assumptions:

    - Simulation begins from 0 seconds.
    - The cafe has unlimited ingredients.
    - If a customer arrives at time t, they place the order at time t, and the coffee starts getting prepared at time t+1.
    - If a customer arrives at time t, and they have tolerance time tol => they collect their order only if it gets done on or before t + tol.
    - Once an order is completed, the customer picks it up and leaves instantaneously.
    - If a customer was already waiting, once a barista finishes their previous order, say at time t, they can start making the order of the waiting customer at time t+1.
    - The cafe has infinite seating capacity.

3. Input format (all space separated):

   - The first line contains B K N The next K lines contain c t_c The next N lines contain i x t_arr_i tol_i.

4. Output format:

    - When a customer c arrives, print [white colour] Customer c arrives at t second(s)

    - When a customer c places their order, print [yellow colour] Customer c orders an espresso
    - When a barista b begins preparing the order of customer c, print [cyan colour] Barista b begins preparing the order of customer c at t second(s)

    - When a barista b successfully complete the order of customer c, print [blue colour] Barista b successfully completes the order of customer c
    - If a customer successfully gets their order, print [green colour] Customer c leaves with their order at t second(s)
    - If a customer’s patience runs out, print [red colour] Customer c leaves without their order at t second(s)
