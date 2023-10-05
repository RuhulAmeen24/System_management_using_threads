// include libraries
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>

time_t start_time;

struct pizza
{
    int pizza_id;
    int preparation_time;
    int ingredients_count;
    int *ingredients;
};

struct customer
{
    int arrival_time;
    int pizza_count;
    int *pizza_ids;
};

struct pizza_order
{
    int pizza_id;
    int customer_id;
    int order_no;
};

struct pizza_pickup
{
    int pizza_id;
    int customer_id;
    int order_no;
};

typedef struct pizza *pizza;
typedef struct customer *customer;
typedef struct pizza_order *pizza_order;
typedef struct pizza_pickup *pizza_pickup;

struct arguments
{
    int number_chefs;
    int number_customers;
    int number_ingredients;
    int number_pizza_varities;
    int number_ovens;
    int time_to_pickup;
    pizza pizzas;
    customer customers;
    int *ingredients_count;
    int *chef_entry_time;
    int *chef_exit_time;
    int current_customer;
    int current_chef;
} glob_arg;

pthread_mutex_t chef_arg;
pthread_mutex_t customer_arg;
pthread_mutex_t order_mutex;
pthread_mutex_t pizza_mutex;
pthread_mutex_t ingredients_valid;
pthread_mutex_t oven_area;
pthread_mutex_t pickup_area;

sem_t orders_available;
sem_t pizzas_available;
sem_t ovens_available;

pizza_order orders_pizza;
pizza_pickup pickup_pizza;

int *pizza_placed;
int *order_placed;
int current_pizza;
int current_order = 0;
int taken_order = 0;
int taken_pizza = 0;
int orders_left = 0;
int pizzas_left;
int total_pizzas = 0;
int pizzas_done = 0;

int *ovens_free;
int current_oven = 0;

void *chef_func(void *arguments)
{
    pthread_mutex_lock(&chef_arg);
    int chef_id = glob_arg.current_chef;
    glob_arg.current_chef++;
    pthread_mutex_unlock(&chef_arg);

    sleep(glob_arg.chef_entry_time[chef_id]);
    // print time chef enters
    // print in blue
    printf("\033[1;34m");
    printf("%ld: Chef %d enters\n", time(NULL) - start_time, chef_id);
    printf("\033[0m");
    time_t chef_start_time = time(NULL);

    while (time(NULL)-start_time<glob_arg.chef_exit_time[chef_id])
    {
        // print chef id with pizzas left
        // printf("chef %d: %d\n", chef_id, pizzas_left);
        if (pizzas_left > 0)
        {
            // print chef waiting for order
            // print in blue
            // printf("\033[1;34m");
            // printf("%ld: Chef %d waiting for order\n", time(NULL) - start_time, chef_id);
            // printf("\033[0m");
            sem_wait(&pizzas_available);
            pthread_mutex_lock(&pizza_mutex);
            int pizza_id = pizza_placed[taken_pizza];
            int order_no = orders_pizza[taken_pizza].order_no;
            int customer_id = orders_pizza[taken_pizza].customer_id;
            // print pizza picked up
            // print in red
            printf("\033[1;31m");
            printf("%ld: Pizza %d is picked up by Chef %d for Order %d for Customer %d\n", time(NULL) - start_time, pizza_id, chef_id, order_no, customer_id);
            printf("\033[0m");
            taken_pizza++;
            pizzas_left--;
            pthread_mutex_unlock(&pizza_mutex);

            int prep_time = glob_arg.pizzas[pizza_id - 1].preparation_time;
            // print chef is preparing pizza
            // print in blue
            printf("\033[1;34m");
            printf("%ld: Chef %d is preparing Pizza %d from Order %d\n", time(NULL) - start_time, chef_id, pizza_id, order_no);
            printf("\033[0m");

            sleep(3);

            sem_wait(&ovens_available);
            pthread_mutex_lock(&oven_area);
            int oven_id = ovens_free[current_oven];
            current_oven--;
            pthread_mutex_unlock(&oven_area);

            // print chef put pizza in oven
            // print in blue
            printf("\033[1;34m");
            printf("%ld: Chef %d puts Pizza %d in Oven %d for order %d\n", time(NULL) - start_time, chef_id, pizza_id, oven_id, order_no);
            printf("\033[0m");

            sleep(prep_time - 3);

            // print chef took pizza out of oven
            // print in blue
            printf("\033[1;34m");
            printf("%ld: Chef %d takes Pizza %d out of Oven %d for order %d\n", time(NULL) - start_time, chef_id, pizza_id, oven_id, order_no);
            printf("\033[0m");

            pthread_mutex_lock(&oven_area);
            current_oven++;
            ovens_free[current_oven] = oven_id;
            pthread_mutex_unlock(&oven_area);
            sem_post(&ovens_available);

            pthread_mutex_lock(&pickup_area);
            pickup_pizza[pizzas_done].pizza_id = pizza_id;
            pickup_pizza[pizzas_done].customer_id = customer_id;
            pickup_pizza[pizzas_done].order_no = order_no;
            pizzas_done++;
            pthread_mutex_unlock(&pickup_area);



            // printf("pizzas left: %d\n", pizzas_left);

        }

        // print time pizza is alloted to chef
    }
    // print time chef exits
    // print in blue
    printf("\033[1;34m");
    printf("%ld: Chef %d exits\n", time(NULL) - start_time, chef_id);
    printf("\033[0m");
}

void *customer_func(void *arguments)
{
    pthread_mutex_lock(&customer_arg);
    int customer_id = glob_arg.current_customer;
    glob_arg.current_customer++;
    pthread_mutex_unlock(&customer_arg);

    sleep(glob_arg.customers[customer_id].arrival_time);

    // print time customer arrives
    // print in yellow
    printf("\033[1;33m");
    printf("%ld: Customer %d arrives\n", time(NULL) - start_time, customer_id);
    printf("\033[0m");

    pthread_mutex_lock(&order_mutex);
    int order_id = current_order;
    current_order++;
    order_placed[order_id] = customer_id;
    pthread_mutex_unlock(&order_mutex);
    sem_post(&orders_available);

    // print in red
    printf("\033[1;31m");
    // print time order placed by customer
    printf("%ld: Order %d is placed by customer %d has pizzas{", time(NULL) - start_time, order_id, customer_id);
    for (int i = 0; i < glob_arg.customers[customer_id].pizza_count; i++)
    {
        printf("%d", glob_arg.customers[customer_id].pizza_ids[i]);
        if (i != glob_arg.customers[customer_id].pizza_count - 1)
            printf(",");
    }
    printf("}\n");
    printf("\033[0m");

    int *previous_ingredients = (int *)malloc(sizeof(int) * glob_arg.number_ingredients);
    for (int i = 0; i < glob_arg.number_ingredients; i++)
        previous_ingredients[i] = glob_arg.ingredients_count[i];

    pthread_mutex_lock(&ingredients_valid);
    for (int i = 0; i < glob_arg.customers[customer_id].pizza_count; i++)
    {
        int pizza_id = glob_arg.customers[customer_id].pizza_ids[i];
        // printf("pizzaid=%d,", pizza_id);
        for (int j = 0; j < glob_arg.pizzas[pizza_id - 1].ingredients_count; j++)
        {
            int ingredient_id = glob_arg.pizzas[pizza_id - 1].ingredients[j];
            // printf("ing_id=%d,", ingredient_id);
            // printf("ing_count before=%d,", glob_arg.ingredients_count[ingredient_id-1]);
            glob_arg.ingredients_count[ingredient_id - 1]--;
            // printf("ing_count after=%d,", glob_arg.ingredients_count[ingredient_id-1]);
            if (glob_arg.ingredients_count[ingredient_id - 1] < 0)
            {
                // print in red
                printf("\033[1;31m");
                // print order no is rejected
                printf("%ld: Order %d is rejected\n", time(NULL) - start_time, order_id);
                printf("\033[0m");
                // print in yellow
                printf("\033[1;33m");
                // print customer exits drive thru zone
                printf("%ld: Customer %d exits drive thru zone\n", time(NULL) - start_time, customer_id);
                printf("\033[0m");
                for (int i = 0; i < glob_arg.number_ingredients; i++)
                    glob_arg.ingredients_count[i] = previous_ingredients[i];
                pthread_mutex_unlock(&ingredients_valid);
                return NULL;
            }
            // printf("\n");
        }
    }
    pthread_mutex_unlock(&ingredients_valid);

    pthread_mutex_lock(&pizza_mutex);

    for (int i = 0; i < glob_arg.customers[customer_id].pizza_count; i++)
    {
        pizza_placed[current_pizza] = glob_arg.customers[customer_id].pizza_ids[i];
        orders_pizza[current_pizza].order_no = order_id;
        orders_pizza[current_pizza].customer_id = customer_id;
        orders_pizza[current_pizza].pizza_id = glob_arg.customers[customer_id].pizza_ids[i];
        current_pizza++;
        sem_post(&pizzas_available);
        pizzas_left++;

    }
    // printf("pizzas_left=%d\n", pizzas_left);
    
    pthread_mutex_unlock(&pizza_mutex);

    sleep(glob_arg.time_to_pickup);

    int pizzas_needed = glob_arg.customers[customer_id].pizza_count;

    while(pizzas_needed>0)
    {
        pthread_mutex_lock(&pickup_area);
        for(int i=0;i<pizzas_done;i++)
        {
            if(pickup_pizza[i].customer_id==customer_id)
            {
                // print in yellow
                printf("\033[1;33m");
                // print time customer picks up pizza
                printf("%ld: Customer %d picks up pizza %d of order %d\n", time(NULL) - start_time, customer_id, pickup_pizza[i].pizza_id, pickup_pizza[i].order_no);
                printf("\033[0m");
                pickup_pizza[i].customer_id=-1;
                pizzas_needed--;
                break;
            }
        }
        pthread_mutex_unlock(&pickup_area);
        nanosleep((const struct timespec[]){{0, 100L}}, NULL);
    }
    // print in yellow
    printf("\033[1;33m");
    // print time customer exits drive thru zone
    printf("%ld: Customer %d exits drive thru zone\n", time(NULL) - start_time, customer_id);
    printf("\033[0m");
}

int main()
{
    // read input
    scanf("%d %d %d %d %d %d", &glob_arg.number_chefs, &glob_arg.number_pizza_varities, &glob_arg.number_ingredients, &glob_arg.number_customers, &glob_arg.number_ovens, &glob_arg.time_to_pickup);

    glob_arg.pizzas = (pizza)malloc(glob_arg.number_pizza_varities * sizeof(struct pizza));
    glob_arg.customers = (customer)malloc(glob_arg.number_customers * sizeof(struct customer));
    glob_arg.ingredients_count = (int *)malloc(glob_arg.number_ingredients * sizeof(int));
    glob_arg.chef_entry_time = (int *)malloc(glob_arg.number_chefs * sizeof(int));
    glob_arg.chef_exit_time = (int *)malloc(glob_arg.number_chefs * sizeof(int));

    // read pizzas
    for (int i = 0; i < glob_arg.number_pizza_varities; i++)
    {
        scanf("%d %d %d", &glob_arg.pizzas[i].pizza_id, &glob_arg.pizzas[i].preparation_time, &glob_arg.pizzas[i].ingredients_count);
        glob_arg.pizzas[i].ingredients = (int *)malloc(glob_arg.pizzas[i].ingredients_count * sizeof(int));
        for (int j = 0; j < glob_arg.pizzas[i].ingredients_count; j++)
        {
            scanf("%d", &glob_arg.pizzas[i].ingredients[j]);
        }
    }

    // take input for ingredients
    for (int i = 0; i < glob_arg.number_ingredients; i++)
    {
        scanf("%d", &glob_arg.ingredients_count[i]);
    }

    // take input for chef entry and exit time
    for (int i = 0; i < glob_arg.number_chefs; i++)
    {
        scanf("%d %d", &glob_arg.chef_entry_time[i], &glob_arg.chef_exit_time[i]);
    }

    // take input for customers
    for (int i = 0; i < glob_arg.number_customers; i++)
    {
        scanf("%d %d", &glob_arg.customers[i].arrival_time, &glob_arg.customers[i].pizza_count);
        total_pizzas += glob_arg.customers[i].pizza_count;
        glob_arg.customers[i].pizza_ids = (int *)malloc(glob_arg.customers[i].pizza_count * sizeof(int));
        for (int j = 0; j < glob_arg.customers[i].pizza_count; j++)
        {
            scanf("%d", &glob_arg.customers[i].pizza_ids[j]);
        }
    }

    glob_arg.current_customer = 0;
    glob_arg.current_chef = 0;

    orders_left = glob_arg.number_customers;
    pizzas_left = 0;

    order_placed = (int *)malloc(glob_arg.number_customers * sizeof(int));
    pizza_placed = (int *)malloc(total_pizzas * sizeof(int));

    orders_pizza = (pizza_order)malloc(total_pizzas * sizeof(struct pizza_order));
    pickup_pizza = (pizza_pickup)malloc(total_pizzas * sizeof(struct pizza_pickup));

    ovens_free = (int *)malloc(glob_arg.number_ovens * sizeof(int));
    for (int i = 0; i < glob_arg.number_ovens; i++)
    {
        ovens_free[i] = glob_arg.number_ovens - i;
    }

    current_oven = glob_arg.number_ovens - 1;

    pthread_mutex_init(&chef_arg, NULL);
    pthread_mutex_init(&customer_arg, NULL);

    pthread_mutex_init(&order_mutex, NULL);
    pthread_mutex_init(&pizza_mutex, NULL);

    pthread_mutex_init(&ingredients_valid, NULL);

    pthread_mutex_init(&oven_area, NULL);
    pthread_mutex_init(&pickup_area, NULL);

    sem_init(&orders_available, 0, 0);
    sem_init(&pizzas_available, 0, 0);
    sem_init(&ovens_available, 0, glob_arg.number_ovens);

    // print in pink
    printf("\033[1;35m");
    printf("Simulation starts\n");
    printf("\033[0m");

    start_time = time(NULL);

    // create threads for chefs
    pthread_t *chefs = (pthread_t *)malloc(glob_arg.number_chefs * sizeof(pthread_t));
    for (int i = 0; i < glob_arg.number_chefs; i++)
    {
        pthread_create(&chefs[i], NULL, chef_func, NULL);
    }

    // create threads for customers
    pthread_t *customers = (pthread_t *)malloc(glob_arg.number_customers * sizeof(pthread_t));
    for (int i = 0; i < glob_arg.number_customers; i++)
    {
        pthread_create(&customers[i], NULL, customer_func, NULL);
    }

    // join threads for chefs
    for (int i = 0; i < glob_arg.number_chefs; i++)
    {
        pthread_join(chefs[i], NULL);
    }

    // join threads for customers
    for (int i = 0; i < glob_arg.number_customers; i++)
    {
        pthread_join(customers[i], NULL);
    }

    // mutex destroy
    pthread_mutex_destroy(&chef_arg);
    pthread_mutex_destroy(&customer_arg);

    pthread_mutex_destroy(&order_mutex);
    pthread_mutex_destroy(&pizza_mutex);

    pthread_mutex_destroy(&ingredients_valid);

    pthread_mutex_destroy(&oven_area);
    pthread_mutex_destroy(&pickup_area);

    // sem destroy
    sem_destroy(&orders_available);
    sem_destroy(&pizzas_available);
    sem_destroy(&ovens_available);

    // print in pink
    printf("\033[1;35m");
    printf("Simulation ends\n");
    printf("\033[0m");

    // free memory
    free(glob_arg.pizzas);
    free(glob_arg.customers);
    free(glob_arg.ingredients_count);
    free(glob_arg.chef_entry_time);
    free(glob_arg.chef_exit_time);
    free(chefs);
    free(customers);
}