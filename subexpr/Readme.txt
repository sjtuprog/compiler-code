1. All flags are implemented. Therefore, driver.cc is also included.

2. do not use flag ssa. My ssa has bugs on for loop. It is not reliable. 
   But this project just goes fine without ssa. My implementation doesn't need ssa.
   I fixed some bugs in my previous ssa.cc file. However, that's not enough.
   (actually flag ssa is disabled by me.)

3. The design of algorithm is very similar to the slides. 
   
   First we split the expression into binary expressions.
   Then we calculate available expression.
   We perform statement level GCSE. 
   
   For copy propagation, since we perform GCSE on statement level and in my implementation,
   there seems to be no duplications. So I only transform the statements back into their original form.(a reverse for splitting expressions).      
   

4. It should be able to pass most of the test cases on the course website.



