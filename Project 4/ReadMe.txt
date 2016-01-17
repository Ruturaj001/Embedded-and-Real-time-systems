An embedded, stand-alone QNX Neutrino program to simulate the workflow
in a typical banking environment -- single queue with queuing to a 
multi-threaded server.

Problem Statement:

Customers enter the bank to transact business on a regular basis. Each new customer arrives every one to four minutes, based on a uniform random distribution. Each new customer enters a single queue of all customers.

Three tellers are available to service customers in the queue. As tellers become available, customers leave the queue, approach the teller and conduct their business. Each customer requires between 30 seconds and 6 minutes for their transaction with the teller. The time required for each transaction is based on a uniform random distribution.

The bank is open for business between the hours of 9:00am and 4:00pm. Customers begin entering when the bank opens in the morning, and stop entering when the bank closes in the afternoon. Customers in the queue at closing time remain in the queue until tellers are available to complete their transactions.

Metrics:

To monitor the performance of the business, metrics are gathered and reported at the end of the day. The metrics are:

The total number of customers serviced during the day.

The average time each customer spends waiting in the queue.

The average time each customer spends with the teller.

The average time tellers wait for customers.

The maximum customer wait time in the queue.

The maximum wait time for tellers waiting for customers.

The maximum transaction time for the tellers.

The maximum depth of the queue.

Design Constraints:

You are free to use any QNX supported concurrency mechanism to implement this lab. Be sure to describe your selected architecture in your lab report.

You may want to create a semaphore to communicate the number of available tellers but other options including QNX message sending and reply blocking are also options.

Each teller is modeled as an independent thread in the Teller Server.

The simulation parameters as described in the Problem Statement can be “hard-coded” as internal constants. However, only define those values in one location – a header file is strongly recommended.

The simulation time is scaled such that 100 milliseconds of absolute clock time represents 1 minute of simulation clock. Therefore, this program will run about 7 hours x 60 minutes /hour * 0.1 seconds / minute.

The output must be presented in simulation clock time (9 AM through closing time around 4 PM).

In order to get the timing right I strongly recommend using timers and then waiting for the timer to complete. Using CPU consuming library functions like usleep are discouraged. Substantially inaccurate simulations will be penalized.

Graduate Students:

Include random breaks for each of the tellers.

Each teller will take a break every 30 to 60 minutes for 1 to 4 minutes. If a teller break time occurs while serving a customer they will go on break as soon as they finish the current customer.

The next break for a teller occurs from 30 to 60 minutes from when they started their previous break.

A break can only occur after the completion of the current customer transaction.

The break timing and duration is based on a random uniform distribution.
