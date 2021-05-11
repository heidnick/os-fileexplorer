#include <iostream>
#include <SDL.h>
#include <dirent.h>
#include <vector>

#define WIDTH 800
#define HEIGHT 600


void initialize(SDL_Renderer *renderer);
void render(SDL_Renderer *renderer);

class FileEntry {
    public:
        void setName(std::string n) {
            name = n;
        }
        void setType(std::string t) {
            type = t;
        }
        std::string getName() {
            return name;
        }
        std::string getType() {
            return type;
        }

    protected:
        std::string name;
        std::string type;
};

int main(int argc, char **argv)
{
    std::vector<FileEntry> file_entries;
    
    char *home = getenv("HOME");
    printf("HOME: %s\n", home);
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir ("/home/nheid/Desktop/..")) != NULL) {
        /* print all the files and directories within directory */
        while ((ent = readdir (dir)) != NULL) {
            if (ent->d_ino != 262146) {
                //printf ("%s\n", ent->d_name);
                FileEntry entry;
                entry.setName(ent->d_name);
                if (ent->d_type == DT_DIR) {
                    entry.setType("directory");
                }else {
                    entry.setType("file");
                }
                file_entries.push_back(entry);
            }
        }
        closedir (dir);
    } else {
        /* could not open directory */
        perror ("");
        return EXIT_FAILURE;
    }

    for (int i=0; i<5; i++) {
        std::cout << file_entries[i].getName() << " " << file_entries[i].getType() << std::endl;
    }

    // initializing SDL as Video
    SDL_Init(SDL_INIT_VIDEO);

    // create window and renderer
    SDL_Renderer *renderer;
    SDL_Window *window;
    SDL_CreateWindowAndRenderer(WIDTH, HEIGHT, 0, &window, &renderer);

    // initialize and perform rendering loop
    initialize(renderer);
    render(renderer);
    SDL_Event event;
    SDL_WaitEvent(&event);
    while (event.type != SDL_QUIT)
    {
        //render(renderer);
        SDL_WaitEvent(&event);
    }

    // clean up
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

void initialize(SDL_Renderer *renderer)
{
    // set color of background when erasing frame
    SDL_SetRenderDrawColor(renderer, 235, 235, 235, 255);
}

void render(SDL_Renderer *renderer)
{
    // erase renderer content
    SDL_RenderClear(renderer);
    
    // TODO: draw!

    // show rendered frame
    SDL_RenderPresent(renderer);
}

