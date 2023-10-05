# Algorithm
* each chef and customer are seperate threads
* communication between threads is done through global variables
* there is a mutex lock to take arguments for each thread
* there is a mutex lock to placing/taking the order and the pizza in the global buffer
* there is a mutex lock to use ovens
* A semaphore is used to keep track of the number of ovens available
* A semaphore is used to keep track of the number of pizza orders placed in the global buffer
* there is a mutex lock to use the the ingredients


1. * if the pizza spots are full in pickup spot, the chef routes the pizzas to secondary storage
   * so once a pizza is picked up, the chef can place a pizza from secondary storage in the pickup spot
   * this can be done by using a mutex lock to access the secondary storage and pickup spot
   * a semaphore is used to keep track of free spots in the pickup spot
  
2. * in this model, when the order is placed, we check whether each pizza in the order has the ingredients to make it
   * if the pizza has the ingredients, we place the order in the global buffer
   * if the pizza does not have the ingredients, we reject the order immediately
   * so in this scenario, our model is already optimized, so it won't affect the ratings of the restaurant

3. * if we know the ingredients to be bought and is arriving time from the super market.
   * we will pretend that the ingredients are already in the restaurant
   * we will accept orders with these ingredients in mind
   * we will wait for the ingredients to arrive and then start assigning the pizzas to the chefs
   * above can be done by using a mutex lock to access the ingredients
   * if the ingredients are not available, we will reject the order immediately