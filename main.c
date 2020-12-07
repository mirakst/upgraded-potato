/*
 * Navne: Admir Bevab, Daniel Berwald, Anders Mariegaard, Nikolaj Dam, Mikkel Stidsen, Torbjørn Helstad, Kasper Henningsen.
 * Mails: SW1A405a@student.aau.dk
 * Gruppe: A405a
 * Studieretning: Software 
 */
#include "main.h"

/* Få lavet task-menu? */

int main(void) {
    User user;
    char cmd_input[INPUT_MAX];
    double active_data[HOURS_PER_DAY + 6][2];
    task task_list[TASK_AMOUNT_MAX];
    int task_amount = 0,
        task_id = 0,
        cmd,
        current_day;

    Test_All();
    Initialize(active_data, &user, task_list, &task_amount, &current_day);

    do {
        cmd = Get_User_Input(cmd_input);

        switch (cmd) {
        case _exit:
            printf("Exiting...\n"); break;
        case help:
            Help_All(); break;
        case help_prices:
            Help_Prices(1); break;
        case help_tasks:
            Help_Tasks(1); break;
        case help_settings:
            Help_Settings(1); break;
        case settings:
            Print_Settings(&user);
            Calculate_kWh_Data(active_data, 0, current_day, user.xXxcumShotxXx); break;
        case save_user:
            Save(user, task_list, task_amount); break;
        case list_data:
            List_kWh_Data(active_data, user.xXxcumShotxXx); break;
        case change_day:
            Change_Day(active_data, &current_day, user.xXxcumShotxXx); break;
        case list_tasks:
            Print_Task_List(task_list, task_amount, Day_To_Weekday(current_day)); break;
        case add_task:
            Add_Task(task_list, &task_amount); break;
        
        case remove_task:
            if (sscanf(cmd_input, " %*s %*s %d", &task_id) == 1)
                Remove_Task(task_list, &task_amount, task_id);
            else
                printf("Task ID not specified.\n");
            break;

        case edit_task:
            if (sscanf(cmd_input, " %*s %*s %d", &task_id) == 1)
                Edit_Task(task_list, task_amount, task_id);
            else
                printf("Task ID not specified.\n");
            break;

        case suggest:
            Suggest_Day (user, task_list, task_amount, active_data, current_day); break;
        case suggest_year:
            Suggest_Year (user, task_list, task_amount, active_data, current_day); break;
            
        default:
            printf("Command not recognized. Type 'help for the list of commands.\n"); break;
        }
    } while (cmd != _exit);

    Save(user, task_list, task_amount);

    return EXIT_SUCCESS;
}

/** Performs all necessary initialization and loading of structs and variables.
 *  @param p[i/o] Unsorted array of prices.
 *  @param p_sorted[i/o] Sorted array of prices.
 *  @param user[i/o] Pointer to the user structure with all user details.
 *  @param task_list[i/o]  Active array of tasks. 
 *  @param task_amount[i/o] Amount of non-empty tasks in the task array. */
void Initialize(double data[][2], User *user, task task_list[TASK_AMOUNT_MAX], int *task_amount, int *current_day) {
    time_t t = time(NULL);
    struct tm time = *localtime(&t);
    int file_status = 0,
        i;
    char *weekday[7] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
         *current_day = time.tm_yday;
    
    /* Attempts to load price and emission data. Terminates if there are none */
    file_status = Calculate_kWh_Data(data, 0, *current_day, user->xXxcumShotxXx);
    if (!file_status) {
        printf("Could not load data. Exiting...\n");
        exit(EXIT_FAILURE);
    }

    /* Attempt to load user details. Starts setup if there is no file */
    user->has_set_hours = 0;
    user->has_name = 0;

    file_status = Load_User_Details(user);
    if (file_status != 1) {
        Print_Welcome();
        First_Time_Setup(user);
        Save_User_Details(*user);
    }
    user->has_set_hours = file_status;
    user->has_name = file_status;

    /* Resets all tasks in the task array */
    for (i = 0; i < TASK_AMOUNT_MAX; i++)
        Reset_Task(&task_list[i]);
    
    /* Attempts to load tasks from tasklist file */
    file_status = Load_Tasks(task_list, task_amount, FILE_TASKLIST);
    if (file_status == 1)
        printf("Loaded %d tasks successfully.\n", *task_amount);
    else if (file_status == -1)
        printf("Failed to load task configuration file: %s.\n", FILE_TASKLIST);
    
    if (user->has_name)
        printf("Hello %s\n", user->user_name);
    printf("Today is %s, %.2d-%.2d-%.2d\n", weekday[time.tm_wday], time.tm_mday, time.tm_mon + 1, time.tm_year + 1900);
}

/** Saves the user details and task list.
 *  @param user[i] User structure with all user details.
 *  @param task_list[i] Active array of task structures.
 *  @param task_amount[i] Amount of non-empty task structures in the task array. */
void Save(User user, task task_list[TASK_AMOUNT_MAX], int task_amount) {
    int save_res = Save_User_Details(user);
    if (save_res != 1) {
        printf("Failed to save user settings. Error code: %d\n", save_res);
        return;
    }
    save_res = Save_Tasks(task_list, task_amount, FILE_TASKLIST);
    if (save_res == -1) {
        printf("Failed to save tasks. Error code %d\n", save_res);
        return;
    }
    printf("Saved tasks and settings successfully.\n");
}

/** Suggests a day for specific tasks
 *  @param user[i] User structure with all user details.
 *  @param task_list[][i] Array of task structures.
 *  @param task_amount[i] Amount of task structures in the task array.
 *  @param p[][][i] Unsorted array of prices.
 *  @param current_day[i] The day that prices are loaded from. Can be edited for debugging. */
void Suggest_Day (User user, task task_list[TASK_AMOUNT_MAX], int task_amount, double data[][2], int current_day) {
    int *assigned_hours = calloc(HOURS_PER_DAY, sizeof(int));
    int i;

    for (i = 0; i < task_amount; i++) {
        task_list[i].max_value = 0;
        task_list[i].min_value = 0;
        if (!task_list[i].days[Day_To_Weekday(current_day)]) {
            continue;
        }
        Find_Start_Hour (user, &task_list[i], assigned_hours, data, current_day, 0);
    }
    Print_Suggestions_Day (task_amount, task_list, Day_To_Weekday(current_day), user.xXxcumShotxXx);
}

/** Calculates the savings if the plan is followed for a year
 *  @param user[i] User structure with all user details.
 *  @param task_list[][i] Array of task structures.
 *  @param task_amount[i] Amount of task structures in the task array.
 *  @param p[][][i] Unsorted array of prices.
 *  @param current_day[i] The day that prices are loaded from. Can be edited for debugging. */
void Suggest_Year (User user, task task_list[TASK_AMOUNT_MAX], int task_amount, double data[][2], int current_day) {
    int *assigned_hours = calloc(HOURS_PER_DAY, sizeof(int));
    int i, day;
    
    /* Reset the relevant task variables */
    for (i = 0; i < task_amount; i++) {
        task_list[i].max_value = 0;
        task_list[i].min_value = 0;
        task_list[i].total_days_yr = 0;
    }

    for (day = 1; day <= DAYS_PER_YEAR; day++) {
        Calculate_kWh_Data(data, 0, day, user.xXxcumShotxXx);

        for (i = 0; i < task_amount; i++) {
            if (!task_list[i].days[Day_To_Weekday(day)])
                continue;
            Find_Start_Hour (user, &task_list[i], assigned_hours, data, current_day, 1);
            task_list[i].total_days_yr++;
        }
    }
    Print_Suggestions_Year (task_amount, task_list, user.xXxcumShotxXx);
    Calculate_kWh_Data (data, 0, current_day, user.xXxcumShotxXx);
}

/** Finds the starting hour with the lowest average price/emission over the task duration.
 *  @param user[i] User structure with all user details.
 *  @param p_task[i/o] Pointer to a task structure.
 *  @param assigned_hours[][i] Sees if there is an active task on the hour.
 *  @param p[][][i] Unsorted array of prices.
 *  @param do_year[i] Boolean to determine whether the yearly or daily savings should be calculated (true = 1, false = 0). */
void Find_Start_Hour (User user, task *p_task, int assigned_hours[HOURS_PER_DAY], double data[][2], int day, int do_year) {
    int i, j,
        duration,
        end_hr = 0,
        best_hr_start = 0,
        best_hr_end = 0,
        should_skip_hr = 0;
    double cur_value = 0.0,
           value_avg = 0.0,
           value_avg_max = 0.0,
           value_avg_min = FUCKING_BIG_ASS_NUMBER,
           value_min = 0.0, 
           value_max = 0.0;

    duration = (int) ceil(p_task->duration);

    for (i = 0; i < HOURS_PER_DAY; i++) {
        /* Calculate end hour for the task and reset variables for each hour */
        if (day == 365)
            end_hr = Wrap_Hour(i + duration);
        else
            end_hr = i + duration;
        
        should_skip_hr = 0;
        cur_value = 0.0;

        should_skip_hr = Should_Skip_Hour(user, p_task, assigned_hours, day, i, end_hr, do_year);

        /* If the user is available in this hour, calculate the avg price values and attempt to assign them */
        if(!should_skip_hr) {
            for (j = 0; j < duration; j++) {
                /* Handle 'overflowing' hours */
                /* Eks. duration = 3, p_task->duration = 2.5, så skal den sidste times pris ganges med den resterende længde af tasken (0.5 timer) */
                if (p_task->duration > j && p_task->duration < (j + 1)) 
                    cur_value += p_task->power * (p_task->duration - j) * data[i + j][0];
                else
                    cur_value += p_task->power * data[i + j][0];
            }
            
            value_avg = cur_value / duration;

            if (value_avg > value_avg_max) {
                value_avg_max = value_avg;
                value_max = cur_value;
            }
            if (value_avg < value_avg_min) {
                value_avg_min = value_avg;
                value_min = cur_value;
                best_hr_start = i;
                best_hr_end = Wrap_Hour(best_hr_start + j);
            }
        }
    }
    
    /* If price_min is unchanged, assume that no start hour was found */
    if (value_avg_min == 100 && !do_year) {
        printf("Could not find a suitable start hour for task %s\n", p_task->name);
        return;
    }
    Assign_Task(p_task, best_hr_start, best_hr_end, value_min, value_max, assigned_hours, do_year);
}

/** Checks whether a task can be assigned in the given start_hr and end_hr interval or not.
 *  @param user[i] User structure with all user details.
 *  @param p_task[i] Pointer to a task structure.
 *  @param assigned_hours[][i] Sees if there is an active task on the hour.
 *  @param start_hr[i] The start hour that is being checked for the given task.
 *  @param end_hr[i] The end hour that is being checked for the given task.
 *  @param do_year[i] Boolean to determine whether the yearly or daily savings should be calculated (true = 1, false = 0). */
int Should_Skip_Hour (User user, task *p_task, int assigned_hours[HOURS_PER_DAY], int day, int start_hr, int end_hr, int do_year) {
    int i;

    /* Skip to next start hour if the user is unavailable and does not want to use all hours */
    if (!user.ignore_availability && !user.available_hours[start_hr])
            return 1;
    
    /* Prevent active tasks from ending outside the user's available hours */
    if (!p_task->is_passive) 
        for (i = start_hr; i != end_hr; i++) {
            if (day == 365)
                i = Wrap_Hour(i);
            if ((!user.ignore_availability && !user.available_hours[Wrap_Hour(i)]) || (!do_year ? assigned_hours[Wrap_Hour(i)] : 0)) 
                return 1; 
        }
    return 0;
}

/** Assigns start/end hours and min/max prices to a given task.
 *  @param p_task[o] Pointer to a task structure.
 *  @param start_hr[i] The start hour that is being checked for the given task.
 *  @param end_hr[i] The end hour that is being checked for the given task.
 *  @param price_min[i] The lowest possible price for the task.
 *  @param price_max[i] The highest possible price for the task.
 *  @param assigned_hours[][o] Sees if there is an active task on the hour.
 *  @param do_year[i] Boolean to determine whether the yearly or daily savings should be calculated (true = 1, false = 0). */
void Assign_Task (task *p_task, int start_hr, int end_hr, double price_min, double price_max, int assigned_hours[HOURS_PER_DAY], int do_year) {
    int i;
    
    if (do_year) { 
        p_task->min_value += price_min;
        p_task->max_value += price_max;
    }
    else {
        p_task->start_hr = start_hr;
        p_task->end_hr = end_hr;
        p_task->min_value = price_min;
        p_task->max_value = price_max;

        if (!p_task->is_passive) {
            for (i = start_hr; i != end_hr; i++) {
                Wrap_Hour(start_hr + i);
                assigned_hours[i] = 1;
            }
        }
    }
}

/* Run all tests */
void Test_All(void){
    Test_KW();
    Test_Wrap_Hours();
    Test_Day_To_Weekday();
    Test_Fixed_Percent();
}

/* Test the Wrap_hours function */
void Test_Wrap_Hours(void) {
    int hour[3] = {22, 4, 25},
        expected_hour[3] = {22, 4, 1};

    assert(expected_hour[0] == Wrap_Hour(hour[0]));
    assert(expected_hour[1] == Wrap_Hour(hour[1]));
    assert(expected_hour[2] == Wrap_Hour(hour[2]));
}

/* Test the Day_To_Weekday function */
void Test_Day_To_Weekday(void) {
    int day[5] = {30, 365, 45, 60, 61},
        expected_day[5] = {6, 5, 0, 1, 2};

    assert(expected_day[0] == Day_To_Weekday(day[0]));
    assert(expected_day[1] == Day_To_Weekday(day[1]));
    assert(expected_day[2] == Day_To_Weekday(day[2]));
    assert(expected_day[3] == Day_To_Weekday(day[3]));
    assert(expected_day[4] == Day_To_Weekday(day[4]));
}