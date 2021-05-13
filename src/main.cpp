#include <iostream>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <dirent.h>
#include <vector>
#include <unistd.h>

#define WIDTH 800
#define HEIGHT 600

class FileEntry {
    public:
        SDL_Texture *text_texture;
        SDL_Texture *icon_texture;
        SDL_Rect texture_rect;
        SDL_Rect icon_rect;
        std::string name;
        std::string type;    
};

typedef struct AppData {
    std::vector<FileEntry> file_entries;
    TTF_Font *font;
    SDL_Texture *phrase;
    SDL_Rect phrase_rect;
} AppData;

void initializeFiles(AppData *data_ptr, char *path);
void initialize(SDL_Renderer *renderer, AppData *data_ptr);
void render(SDL_Renderer *renderer, AppData *data_ptr);
std::string findType(std::string file);


int main(int argc, char **argv)
{
    //Initializing file entries
    AppData data;
    char *home = getenv("HOME");
    initializeFiles(&data, home);
    
    // initializing SDL as Video
    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_PNG);
    TTF_Init();

    // create window and renderer
    SDL_Renderer *renderer;
    SDL_Window *window;
    SDL_CreateWindowAndRenderer(WIDTH, HEIGHT, 0, &window, &renderer);

    // initialize and perform rendering loop
    initialize(renderer, &data);
    render(renderer, &data);
    SDL_Event event;
    SDL_WaitEvent(&event);
    while (event.type != SDL_QUIT)
    {
        //render(renderer);
        SDL_WaitEvent(&event);
        switch (event.type)
        {
            // SDL_MOUSEMOTION:
            //    break;
            case SDL_MOUSEBUTTONDOWN:
                break;
            case SDL_MOUSEBUTTONUP:
                break;
        }
    }

    // clean up
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}


/*
    Creates array of FileEntries sorted in alphabetical order
*/
void initializeFiles(AppData *data_ptr, char *path) {
    printf("PATH: %s\n", path);
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir (path)) != NULL) {
        /* print all the files and directories within directory */
        while ((ent = readdir (dir)) != NULL) {
            if (ent->d_ino != 262146) {
                FileEntry entry;
                entry.name = ent->d_name;
                if (ent->d_type == DT_DIR) {
                    entry.type = "directory";
                }else {
                    std::string fullpath = path;
                    fullpath += "/" + entry.name;
                    if (access (fullpath.c_str(), X_OK)) {
                        entry.type = "executable";
                    }else {
                        entry.type = findType(entry.type);
                    }
                }
                //std::cout << entry.type << std::endl;
                data_ptr->file_entries.push_back(entry);
            }
        }
        closedir (dir);
    } else {
        /* could not open directory */
        perror ("");
    }

    //Sorting files alphabetically
    int n = data_ptr->file_entries.size();

    for(int i = 0; i < n - 1; i++) {
        for(int j = 0; j < n - i - 1; j++) {
            if(data_ptr->file_entries[j].name > data_ptr->file_entries[j+1].name) {
                FileEntry temp = data_ptr->file_entries[j];
                data_ptr->file_entries[j] = data_ptr->file_entries[j+1];
                data_ptr->file_entries[j+1] = temp;
            }
        }
    }
} //InitializeFiles

/*
    Returns the type of the file
*/
std::string findType(std::string file) {
    std::vector<std::string> image = {".jpg", ".jpeg", ".png", ".tif", ".tiff", ".gif"};
    std::vector<std::string> video = {".mp4", ".mov", ".mkv", ".avi", ".webm"};
    std::vector<std::string> code = {".h", ".c", ".cpp", ".py", ".java", ".js"};
    for (int i=0; i<image.size(); i++) {
        std::size_t found = file.find(image[i]);
        if (found!=std::string::npos) {
            return "image";
        }
    }
    for (int i=0; i<video.size(); i++) {
        std::size_t found = file.find(video[i]);
        if (found!=std::string::npos) {
            return "video";
        }
    }
    for (int i=0; i<code.size(); i++) {
        std::size_t found = file.find(code[i]);
        if (found!=std::string::npos) {
            return "code";
        }
    }
    return "other";
}

/*
    Creates and queues textures for each file entry
*/
void initialize(SDL_Renderer *renderer, AppData *data_ptr)
{
    // set color of background when erasing frame
    data_ptr->font = TTF_OpenFont("resrc/OpenSans-Regular.ttf", 20);
    SDL_Color color = { 0, 0, 0 };

    for (int i=0; i<data_ptr->file_entries.size(); i++) {
        //Creates and queues texture for the file name   
        const char* name = data_ptr->file_entries[i].name.c_str();
        SDL_Surface *text_surf = TTF_RenderText_Solid(data_ptr->font, name, color);
        data_ptr->file_entries[i].text_texture = SDL_CreateTextureFromSurface(renderer, text_surf);
        SDL_FreeSurface(text_surf);
        data_ptr->file_entries[i].texture_rect.x = 65;
        data_ptr->file_entries[i].texture_rect.y = (i*20);
        SDL_QueryTexture(data_ptr->file_entries[i].text_texture, NULL, NULL, &(data_ptr->file_entries[i].texture_rect.w), 
                         &(data_ptr->file_entries[i].texture_rect.h));

        //creates and queues texture for the file image
        SDL_Surface *img_surf;
        if (data_ptr->file_entries[i].type == "executable") {
            img_surf = IMG_Load("resrc/icon-exe.png");
        }else if(data_ptr->file_entries[i].type == "directory") {
            img_surf = IMG_Load("resrc/icon-folder.png");
        }else if(data_ptr->file_entries[i].type == "image") {
            img_surf = IMG_Load("resrc/icon-image.png");
        }else if(data_ptr->file_entries[i].type == "video") {
            img_surf = IMG_Load("resrc/icon-video.png");
        }else if(data_ptr->file_entries[i].type == "other") {
            img_surf = IMG_Load("resrc/icon-unknown.png");
        }else if(data_ptr->file_entries[i].type == "code") {
            img_surf = IMG_Load("resrc/icon-gear.png");
        }
        data_ptr->file_entries[i].icon_texture = SDL_CreateTextureFromSurface(renderer, img_surf);
        SDL_FreeSurface(img_surf);
        data_ptr->file_entries[i].icon_rect.x = 10;
        data_ptr->file_entries[i].icon_rect.y = i*20;
        data_ptr->file_entries[i].icon_rect.w = 10;
        data_ptr->file_entries[i].icon_rect.h = 10;
        SDL_QueryTexture(data_ptr->file_entries[i].icon_texture, NULL, NULL, &(data_ptr->file_entries[i].icon_rect.w), 
                         &(data_ptr->file_entries[i].icon_rect.h));
    }
}

/*
    Renders file entry array
*/
void render(SDL_Renderer *renderer, AppData *data_ptr)
{
    // erase renderer content
    SDL_SetRenderDrawColor(renderer, 235, 235, 235, 255);
    SDL_RenderClear(renderer);
    
    // TODO: draw!
    for (int i=0; i<data_ptr->file_entries.size(); i++) {
        SDL_RenderCopy(renderer, data_ptr->file_entries[i].icon_texture, NULL, &(data_ptr->file_entries[i].icon_rect));
        SDL_RenderCopy(renderer, data_ptr->file_entries[i].text_texture, NULL, &(data_ptr->file_entries[i].texture_rect));
    }

    // show rendered frame
    SDL_RenderPresent(renderer);
}

