#include <iostream>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <dirent.h>
#include <vector>
#include <unistd.h>
#include <sys/stat.h>
#include <sstream>

#define WIDTH 800
#define HEIGHT 600

class FileEntry {
    public:
        SDL_Texture *text_texture;
        SDL_Texture *size_text_texture;
        SDL_Texture *icon_texture;
        SDL_Rect texture_rect;
        SDL_Rect size_texture_rect;
        SDL_Rect icon_rect;
        std::string name;
        std::string type;
        bool isClicked;
        double size;
        std::string units;
};

typedef struct AppData {
    std::vector<FileEntry> file_entries;
    TTF_Font *font;
    std::string current_path;
    int largest_width;
} AppData;

void initializeFiles(AppData *data_ptr, char *path);
void initialize(SDL_Renderer *renderer, AppData *data_ptr);
void render(SDL_Renderer *renderer, AppData *data_ptr);
std::string findType(std::string file);


int main(int argc, char **argv)
{
    //Initializing file entries
    AppData data;
    char *path = getenv("HOME");
    data.current_path = path;
    initializeFiles(&data, path);
    
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
                if (event.button.button == SDL_BUTTON_LEFT) {
                    int idx_clicked = event.button.y / 22;
                    int high_bound = data.file_entries[idx_clicked].texture_rect.w + 32;
                    if (idx_clicked < data.file_entries.size()) {
                        if (event.button.x >= 10 && event.button.x <= high_bound) {
                            std::cout<<idx_clicked<<std::endl;
                            if (data.file_entries[idx_clicked].type == "directory") {
                                data.current_path += "/" + data.file_entries[idx_clicked].name;
                                std::cout << "curpath init: " << data.current_path << std::endl;
                                char *cstr = new char[data.current_path.length() + 1];
                                strcpy(cstr, data.current_path.c_str());
                                initializeFiles(&data, cstr);
                                delete [] cstr;
                                initialize(renderer, &data);
                            }else {
                                pid_t pid = fork();
                                if (0 == pid) {
                                    data.current_path += "/" + data.file_entries[idx_clicked].name;
                                    char arg0[9];
                                    char arg1[128];
                                    strcpy(arg0, "xdg-open");
                                    //strcpy(arg1, data.current_path.c_str());
                                    strcpy(arg1, data.current_path.c_str());
                                    std::cout << "arg1 " <<  arg1 <<std::endl;
                                    char *const exec_args[3] = {arg0, arg1, NULL};
                                    execvp("xdg-open", exec_args);
                                }
                            }
                        }
                    }
                    break;
                }
            //case SDL_MOUSEBUTTONUP:
            //    break;
        }
        render(renderer, &data);
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
    data_ptr->file_entries.clear();
    printf("PATH: %s\n", path);
    DIR *dir;
    struct dirent *ent;
    struct stat fileInfo;
    if ((dir = opendir (path)) != NULL) {
        /* print all the files and directories within directory */
        while ((ent = readdir (dir)) != NULL) {
            std::string temp = ent->d_name;
            std::size_t found = temp.find(".");
            if (!(found!=std::string::npos && temp.length() == 1)) {
                FileEntry entry;
                entry.name = ent->d_name;
                if (ent->d_type == DT_DIR) {
                    entry.type = "directory";
                }else {
                    std::string fullpath = path;
                    
                    fullpath += "/" + entry.name;
                    stat(fullpath.c_str(), &fileInfo);
                    if (access (fullpath.c_str(), X_OK)) {
                        entry.type = "executable";
                    }else {
                        entry.type = findType(entry.type);
                    }
                    //std::cout << fileInfo.st_size << std::endl;
                    if (fileInfo.st_size >= 1024 && fileInfo.st_size < 1048576) {
                        entry.size = fileInfo.st_size / 1024;
                        entry.units = "KiB";
                    }else if (fileInfo.st_size >= 1048576 && fileInfo.st_size < 1073741824) {
                        entry.size = fileInfo.st_size / 1048576;
                        entry.units = "MiB";
                    }else if (fileInfo.st_size >= 1073741824) {
                        entry.size = fileInfo.st_size / 1073741824;
                        entry.units = "GiB";
                    }else {
                        entry.size = fileInfo.st_size;
                        entry.units = "B";
                    }
                }
                entry.isClicked = false;
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
    int largest_width = 0;

    for (int i=0; i<data_ptr->file_entries.size(); i++) {
        //Creates and queues texture for the file name   
        const char* name = data_ptr->file_entries[i].name.c_str();
        SDL_Surface *text_surf = TTF_RenderText_Solid(data_ptr->font, name, color);
        data_ptr->file_entries[i].text_texture = SDL_CreateTextureFromSurface(renderer, text_surf);
        SDL_FreeSurface(text_surf);
        data_ptr->file_entries[i].texture_rect.x = 32;
        data_ptr->file_entries[i].texture_rect.y = (i*22);
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
        data_ptr->file_entries[i].icon_rect.y = i*22;
        data_ptr->file_entries[i].icon_rect.w = 10;
        data_ptr->file_entries[i].icon_rect.h = 10;
        SDL_QueryTexture(data_ptr->file_entries[i].icon_texture, NULL, NULL, &(data_ptr->file_entries[i].icon_rect.w), 
                         &(data_ptr->file_entries[i].icon_rect.h));
        if (data_ptr->file_entries[i].texture_rect.w > largest_width) {
            largest_width = data_ptr->file_entries[i].texture_rect.w;
        }
    }
    data_ptr->largest_width = largest_width;
    std::cout << largest_width + 32 << std::endl;
    for (int i=0; i<data_ptr->file_entries.size(); i++) {
        if (!(data_ptr->file_entries[i].type == "directory")) {
            std::stringstream ss;
            ss << data_ptr->file_entries[i].size;
            std::string file_size = ss.str();
            file_size += " " + data_ptr->file_entries[i].units;
            const char* fsize = file_size.c_str();
            SDL_Surface *size_text_surf = TTF_RenderText_Solid(data_ptr->font, fsize, color);
            data_ptr->file_entries[i].size_text_texture = SDL_CreateTextureFromSurface(renderer, size_text_surf);
            SDL_FreeSurface(size_text_surf);
            data_ptr->file_entries[i].size_texture_rect.x = 36+ data_ptr->largest_width;
            data_ptr->file_entries[i].size_texture_rect.y = (i*22);
            SDL_QueryTexture(data_ptr->file_entries[i].size_text_texture, NULL, NULL, &(data_ptr->file_entries[i].size_texture_rect.w), 
                            &(data_ptr->file_entries[i].size_texture_rect.h));
        }
        
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
        //size_text_texture is the problem
        if (!(data_ptr->file_entries[i].type == "directory")) {
            SDL_RenderCopy(renderer, data_ptr->file_entries[i].size_text_texture, NULL, &(data_ptr->file_entries[i].size_texture_rect));
        }
    }

    // show rendered frame
    SDL_RenderPresent(renderer);
}

