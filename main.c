#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

int TIME_STEP = 0;
struct Volunteer;
struct Server;


void execute_command(char*);     //riceve una riga dal file ed esegue il comando corrispondente
void initialize_system();
FILE* open_file(FILE *);
int read_file(FILE *fp);
bool max_volunteers_connected(struct Server *);
int disconnect_volunteer(int);   //disconnette un volontario dal sistema
bool seventy_five_percent_capacity_reached(struct Server *s);     //controlla se la capacità supera il 75%
bool capacity_less_than_or_equal_to_25_percent(struct Server *s);    //controlla se la capacità è sotto il 25%
void time_step();    //avanza di un'unità di tempo
void check_for_overloaded_servers();     //se seventy_five_percent_capacity_reached ritorna true, chiama turn_on_server_and_assign_volunteers
void check_for_underloaded_servers();    //se capacity_less_than_or_equal_to_25_percent ritorna true, chiama turn_off_server_in_system
int carry_out_work_unit();       //fa eseguire un'unita di lavoro
int turn_on_server_and_assign_volunteers(struct Server *);     //accende un serve spento e assegna il 50% di volontari di un altro server
void connect_volunteer_to_server(struct Server *, struct Volunteer *);    //riassegna un volontario a un altro server
void connect_volunteer(int);     //connette nuovo volontario a un server
void print_system_status();
int print_node_status();
void assign_work_to_volunteers();     //ogni server accesso assegna ai suoi volontari connessi un'unità di lavoro


typedef struct Server {
    int address;
    int manageable_volunteers;
    int assigned_work_units;
    int completed_work_units;
    int turned_on;
    struct Server* other_servers;
    int connected_volunteers_count;
    int volunteers_array_index;
    int time_step_worked;
    struct Volunteer* connected_volunteers;
} Server;

typedef struct Volunteer {
    int address;
    int completed_work_units;
    bool work_completed;
    bool work_assigned;
    int time_step_worked;
    struct Server* connected_server;
} Volunteer;

typedef struct DistributionSystem {
    int turned_on_servers_count;
    int total_work_unit;
    int max_servers_count;
    int servers_array_index;
    int completed_work_unit;
    Server* available_servers;
}DistributionSystem;

DistributionSystem *distribution_system;





int main() {
    int i;
    FILE *fp = NULL;

    initialize_system();

    fp = open_file(fp);

    read_file(fp);

    fclose(fp);

    return 0;
}



bool max_volunteers_connected(Server *s) {
    if(s->connected_volunteers_count == s->manageable_volunteers) {
        return true;
    }
    else {
        return false;
    }
}

bool seventy_five_percent_capacity_reached(Server *s) {
    float percentage = (float)s->connected_volunteers_count / s->manageable_volunteers * 100.0;
    if(percentage >= 75.0) {
        return true;
    }
    else {
        return false;
    }
}

bool capacity_less_than_or_equal_to_25_percent(Server *s) {
    float percentage = (float)s->connected_volunteers_count / s->manageable_volunteers * 100;
    if(percentage <= 25.0) {
        return true;
    }
    else {
        return false;
    }
}

void connect_volunteer(int address) {
    int i;

    for(i = 0 ; i <= distribution_system->servers_array_index; ++i) {
        Server *server = &distribution_system->available_servers[i];
        if(max_volunteers_connected(server)) {
            continue;
        }
        printf("Connecting volunteer %d to server %d\n", address, server->address);
        server->volunteers_array_index += 1;
        Volunteer *v = &server->connected_volunteers[server->volunteers_array_index];
        v->address = address;
        v->work_assigned = false;
        v->completed_work_units = 0;
        v->work_completed = false;
        v->time_step_worked = TIME_STEP;
        v->connected_server = server;
        server->connected_volunteers_count += 1;
        break;
    }


}

void connect_volunteer_to_server(Server *server, Volunteer *volunteer) {
    int i;

    printf("Migrating volunteer %d from server %d to server %d\n", volunteer->address, volunteer->connected_server->address, server->address);
    volunteer->connected_server = server;
    server->volunteers_array_index += 1;
    server->connected_volunteers_count += 1;
    server->connected_volunteers[server->volunteers_array_index] = *(volunteer);



}


int disconnect_volunteer(int address) {
    int i, j;
    for(i = 0 ; i< distribution_system->turned_on_servers_count; ++i) {
        Server *server = &distribution_system->available_servers[i];
        for(j=0; j< server->connected_volunteers_count; ++j) {
            Volunteer *volunteer = &server->connected_volunteers[j];
            if(volunteer->address == address) {
                printf("Disconnecting volunteer %d from Server %d\n",address, server->address);

                Volunteer temp = server->connected_volunteers[server->volunteers_array_index];
                server->connected_volunteers[server->volunteers_array_index] = *(volunteer);
                *(volunteer) = temp;
                server->volunteers_array_index --;
                server->connected_volunteers_count --;
                break;

            }
        }
    }
}



int carry_out_work_unit() {
    int i, j;
    for( i = 0 ; i < distribution_system->turned_on_servers_count; ++i) {
        Server s = distribution_system->available_servers[i];
        for( j = 0 ;  j < s.connected_volunteers_count; ++j) {
            Volunteer *volunteer = &s.connected_volunteers[j];
            if(volunteer->time_step_worked < TIME_STEP && volunteer->work_assigned) {
                printf("Volunteer %d carrying out work unit and communicating to server %d\n",volunteer->address, s.address);
                volunteer->connected_server->completed_work_units += 1;
                distribution_system->completed_work_unit += 1;
                volunteer->work_assigned = false;
                volunteer->work_completed = false;
                volunteer->completed_work_units += 1;
            }
        }
    }
}

int turn_off_server_in_system(Server* current_server) {
    int i;
    int j;
    int connected_volunteers_to_current_server = current_server->connected_volunteers_count;
    int free_slots_for_volunteers = 0;

    printf("Found Server %d with 25%% or less workload, trying to turn off...\n", current_server->address);

    for(i = 0; i < distribution_system->turned_on_servers_count; ++i) {
        Server server = distribution_system->available_servers[i];

        if(server.address == current_server->address) {
            continue;
        }
        free_slots_for_volunteers += (server.manageable_volunteers - server.connected_volunteers_count);
    }

    if(connected_volunteers_to_current_server > free_slots_for_volunteers) {
        printf("Error: can't turn off the Server %d as not enough free slots to distribute volunteers\n", current_server->address);
        return -1;
    }


    for(i = 0; i < distribution_system->turned_on_servers_count; ++i) {
        Server *server = &distribution_system->available_servers[i];
        if(server->address == current_server->address) {
            continue;
        }
        int available_slots = server->manageable_volunteers - server->connected_volunteers_count;

        for(j = 1; j<= available_slots; ++j) {
            if(connected_volunteers_to_current_server == 0) {
                break;
            }

            connect_volunteer_to_server(server, &current_server->connected_volunteers[current_server->volunteers_array_index]);
            current_server->volunteers_array_index -= 1;
            current_server->connected_volunteers_count -= 1;
            connected_volunteers_to_current_server -= 1;
            available_slots --;
        }
    }

    if(connected_volunteers_to_current_server == 0) {
        Server temp = distribution_system->available_servers[distribution_system->servers_array_index];
        distribution_system->available_servers[distribution_system->servers_array_index] = *(current_server);
        *(current_server) = temp;
        distribution_system->servers_array_index --;
        distribution_system->turned_on_servers_count --;
    }

}

void check_for_underloaded_servers() {
    int i;

    printf("Checking for underloaded servers...\n");

    for(i = 0 ; i < distribution_system->turned_on_servers_count;  ++i) {
        Server *s = &distribution_system->available_servers[i];

        if(s->time_step_worked < TIME_STEP && capacity_less_than_or_equal_to_25_percent(s)) {
            turn_off_server_in_system(s);
        }
    }
}

void time_step() {
    check_for_underloaded_servers();
    check_for_overloaded_servers();
    assign_work_to_volunteers();
    carry_out_work_unit();
    TIME_STEP++;
}

void execute_command(char* str) {
    char *command_name = strtok(str, " ");
    if(command_name[strlen(command_name) - 1] == '\n') {
        command_name[strlen(command_name) - 1] = 0;
    }
    char * argument = strtok(NULL, " ");

    if(argument != NULL) {
        if(argument[strlen(argument) - 1] == '\n') {
            argument[strlen(argument) - 1] = 0;
        }
    }


    if(strcmp(command_name, "WORK_UNITS") == 0) {


        int work_units = atoi(argument);
        printf("\nCOMMAND FOUND: WORK_UNITS %d\n => Setting system work units\n", work_units);
        distribution_system->total_work_unit = work_units;
    }
    else if(strcmp(command_name, "VOLUNTEER_CONNECTED") == 0) {
        printf("\nCOMMAND FOUND: VOLUNTEER_CONNECTED %s\n => ", argument);
        connect_volunteer(atoi(argument));
    }
    else if(strcmp(command_name, "VOLUNTEER_DISCONNECTED") == 0) {
        printf("\nCOMMAND FOUND: VOLUNTEER_DISCONNECTED %s\n => ", argument);
        disconnect_volunteer(atoi(argument));
    }
    else if(strcmp(command_name, "TIME_STEP") == 0) {
        printf("\nCOMMAND FOUND: TIME_STEP \n => Advancing time by 1 unit\n");
        time_step();
    }
    else if(strcmp(command_name, "PRINT_NODE_STATUS") == 0) {
        printf("\nCOMMAND FOUND: PRINT_NODE_STATUS %s\n => ", argument);
        print_node_status(atoi(argument));
    }
    else if(strcmp(command_name, "PRINT_SYSTEM_STATUS") == 0) {
        printf("\nCOMMAND FOUND: PRINT_SYSTEM_STATUS\n => ");
        print_system_status();
    }
}




void check_for_overloaded_servers() {

    printf("Checking for overloaded servers...\n");
    int i;
    for(i = 0 ; i < distribution_system->turned_on_servers_count ; ++i) {
        Server *s = &distribution_system->available_servers[i];
        if(seventy_five_percent_capacity_reached(s)) {
            turn_on_server_and_assign_volunteers(s);
        }
    }
}

void assign_work_to_volunteers() {
    int i, j;
    for( i = 0 ; i < distribution_system->turned_on_servers_count; ++i) {
        Server s = distribution_system->available_servers[i];
        for( j = 0 ;  j < s.connected_volunteers_count; ++j ) {
            Volunteer *volunteer = &s.connected_volunteers[j];
            if(!volunteer->work_assigned) {
                printf("Server %d assigning work to volunteer %d\n", s.address, volunteer->address);
                volunteer->work_assigned = true;
            }
        }
    }
}

void initialize_system() {
    distribution_system = malloc(sizeof(system));
    int i;
    distribution_system->available_servers = malloc(10*sizeof(Server));
    distribution_system->turned_on_servers_count = 1;
    distribution_system->completed_work_unit = 0;
    distribution_system->max_servers_count = 5;
    for(i = 0 ; i < distribution_system->max_servers_count; i++) {
        Server s;
        s.volunteers_array_index = -1;
        s.connected_volunteers_count = 0;
        s.manageable_volunteers = 10;
        s.address = i;
        s.turned_on = 1;
        s.connected_volunteers = malloc(50*sizeof(Volunteer));
        distribution_system->available_servers[i] = s;
    }
}

int turn_on_server_and_assign_volunteers(Server *previous_server) {
    int i;
    Server *new_server;
    int total_volunteer_connected = previous_server->connected_volunteers_count;
    int volunteer_count_to_connected_to_new_server = total_volunteer_connected / 2;
    int number_of_volunteers_assigned = 0;

    if (distribution_system->turned_on_servers_count == distribution_system->max_servers_count) {
        printf("Error: Can't turn on new server, maximum servers limit reached");
        return -1;
    }

    distribution_system->turned_on_servers_count += 1;
    distribution_system->servers_array_index += 1;

     new_server = &distribution_system->available_servers[distribution_system->servers_array_index];
     new_server->volunteers_array_index = -1;
     new_server->time_step_worked = TIME_STEP;

     printf("Server %d found with 75%% volunteers or more...\n Turned on Server %d\n", previous_server->address, new_server->address);
    for(i = previous_server->volunteers_array_index ; i >= 0; i--) {
        if(number_of_volunteers_assigned == volunteer_count_to_connected_to_new_server) {
            break;
        }

        previous_server->connected_volunteers_count -= 1;
        previous_server->volunteers_array_index -= 1;
        connect_volunteer_to_server(new_server, &previous_server->connected_volunteers[i]);
        number_of_volunteers_assigned += 1;
    }



    new_server->turned_on = 1;
}

void print_system_status() {
    int i, j;

    printf("=============== ACTIVE SERVERS ================\n");

    printf("Percentage of work completed: %.2f\n", (float)distribution_system->completed_work_unit / distribution_system->total_work_unit * 100);

    for(i = 0; i < distribution_system->turned_on_servers_count; ++i ) {

        Server server = distribution_system->available_servers[i];
        printf("Server %d:\n", server.address);
        printf("Volunteers connected to Server %d :\n", server.address);
        for(j = 0; j < server.connected_volunteers_count ; ++j ) {
            printf("Volunteer %i\n", server.connected_volunteers[j].address);

        }
    }
}

int print_node_status(int address) {
    int i, j;
    for(i = 0 ; i < distribution_system->turned_on_servers_count ; i++) {
        Server server = distribution_system->available_servers[i];
        for (j = 0 ; j < server.connected_volunteers_count; ++j) {
            Volunteer volunteer = server.connected_volunteers[j];
            if (volunteer.address == address) {
                printf("============== Node Status =====================");
                printf("Volunteer Address: %d\n", volunteer.address);
                printf("Number of work units completed: %d\n", volunteer.completed_work_units);
                printf("Currently work unit assigned: %s\n", volunteer.work_assigned ? "Yes" : "No");
                return 0;
            }
        }
    }
    printf("Error: Node not found");
    return -1;
}




int read_file(FILE *fp) {
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    while ((read = getline(&line, &len, fp)) != -1) {
        execute_command(line);
    }
}

FILE* open_file(FILE *fp) {
    char  filename[50];
    printf("Enter the filename to be opened \n");
    scanf("%s", filename);
    fp = fopen(filename, "r"); // read mode

    if (fp == NULL)
    {
        perror("Error while opening the file.\n");
        exit(EXIT_FAILURE);
    }
    return fp;
}
