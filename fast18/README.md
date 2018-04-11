# The Experiments of the FAST 2018 Paper

The input files in the fast18 folder can be used to repeat the experiments of the FAST 2018 paper [1].

1. **Contention Effect in the SSD Back END:** the 3 workload definition files in the *backend-contention* folder should be fed into MQSim as workload definition files. These files and their corresponding output of execution are described below:<br />
1.1. *workload-backend-flow-1*: response time of flow-1 when it is executed alone<br />
1.2. *workload-backend-flow-2*: response time of flow-2 when it is executed alone<br />
1.3. *workload-backend-flow-1-flow-2*: response time of flow-1 and flow-2 when they are executed concurrently<br />
The response time of flow-1 and flow-2 in the concurrent execution (1.3) should be divided to the response time of flow-1 (1.1) and flow-2 (1.2), respectively, when they are executed alone.

