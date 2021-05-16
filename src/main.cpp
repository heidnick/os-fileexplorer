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
        double size;
        std::string units;
        int dir_contents_idx;
        int x_offset;
};

typedef struct AppData {
    //std::vector<FileEntry> file_entries;
    TTF_Font *font;
    std::string current_path;
    int largest_width;
    int last_y;
    bool rec_toggle;
    SDL_Texture *rec_r_img_texture;
    SDL_Texture *rec_g_img_texture;
    SDL_Texture *rec_text_texture;
    SDL_Rect rec_r_img_rect;
    SDL_Rect rec_g_img_rect;
    SDL_Rect rec_text_rect;
    std::vector<std::vector<FileEntry>> file_entries;
} AppData;

void initializeRecToggle(SDL_Renderer *renderer, AppData *data_ptr);
void initializeFiles(AppData *data_ptr, char *path);
void initRecursive(SDL_Renderer *renderer, AppData *data_ptr, std::string path, int idx, int x_offset);
void initialize(SDL_Renderer *renderer, AppData *data_ptr, int idx);
void render(SDL_Renderer *renderer, AppData *data_ptr);
std::string findType(std::string file);


int main(int argc, char **argv)
{
    //Initializing file entries
    AppData data;
    char *path = getenv("HOME");
    data.current_path = path;
    data.rec_toggle = false;
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
    initialize(renderer, &data, 0);
    initializeRecToggle(renderer, &data);
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
                    //Toggling recursive mode
                    if (event.button.x >= 776 && event.button.x <= 798 && event.button.y >= 2 && event.button.y <= 24) {
                        if (data.rec_toggle == true) {
                            data.rec_toggle = false;
                            char *cstr = new char[data.current_path.length() + 1];
                            strcpy(cstr, data.current_path.c_str());
                            initializeFiles(&data, cstr);
                            delete [] cstr;
                            initialize(renderer, &data, 0);
                            data.last_y = 0;
                        }else {
                            data.rec_toggle = true;
                        }
                    }

                    if (data.rec_toggle == false) {
                        // Navigate through current folder
                        int idx_clicked = event.button.y / 22;
                        if (idx_clicked < data.file_entries[0].size()) {
                            int high_bound = data.file_entries[0][idx_clicked].texture_rect.w + 32;
                            if (event.button.x >= 10 && event.button.x <= high_bound) {
                                if (data.file_entries[0][idx_clicked].type == "directory") {
                                    data.current_path += "/" + data.file_entries[0][idx_clicked].name;
                                    char *cstr = new char[data.current_path.length() + 1];
                                    strcpy(cstr, data.current_path.c_str());
                                    initializeFiles(&data, cstr);
                                    delete [] cstr;
                                    initialize(renderer, &data, data.file_entries.size() - 1);
                                }else {
                                    pid_t pid = fork();
                                    if (0 == pid) {
                                        data.current_path += "/" + data.file_entries[0][idx_clicked].name;
                                        char arg0[9];
                                        char arg1[128];
                                        strcpy(arg0, "xdg-open");
                                        strcpy(arg1, data.current_path.c_str());
                                        char *const exec_args[3] = {arg0, arg1, NULL};
                                        execvp("xdg-open", exec_args);
                                    }
                                }
                            }
                        }
                        break;
                    }else {
                        data.last_y = 0;
                        initRecursive(renderer, &data, data.current_path, 0, 0);
                    }
                } // left buttondown
            //case SDL_MOUSEBUTTONUP:
            //    break;
        } //switch
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
    if (data_ptr->rec_toggle == false) {
        data_ptr->file_entries.clear();
    }
    //printf("PATH: %s\n", path);
    
    DIR *dir;
    struct dirent *ent;
    struct stat fileInfo;
    std::vector<FileEntry> entries;
    if ((dir = opendir (path)) != NULL) {
        // print all the files and directories within directory
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
                //std::cout << "idx: " << idx << std::endl;
                entry.dir_contents_idx = -1;
                entries.push_back(entry);
            }
        }
        closedir (dir);

        data_ptr->file_entries.push_back(entries);
    } else {
        // could not open directory
        std::cout << "error: " << path << std::endl;
        exit(-1);
    }

    //Sorting files alphabetically
    int idx = data_ptr->file_entries.size() - 1;
    int n = data_ptr->file_entries[idx].size();

    for(int i = 0; i < n - 1; i++) {
        for(int j = 0; j < n - i - 1; j++) {
            if(data_ptr->file_entries[idx][j].name > data_ptr->file_entries[idx][j+1].name) {
                FileEntry temp = data_ptr->file_entries[idx][j];
                data_ptr->file_entries[idx][j] = data_ptr->file_entries[idx][j+1];
                data_ptr->file_entries[idx][j+1] = temp;
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
void initialize(SDL_Renderer *renderer, AppData *data_ptr, int idx)
{
    // set color of background when erasing frame
    data_ptr->font = TTF_OpenFont("resrc/OpenSans-Regular.ttf", 20);
    SDL_Color color = { 0, 0, 0 };
    int largest_width = 0;

    for (int i=0; i<data_ptr->file_entries[idx].size(); i++) {

        //Creates and queues texture for the file name   
        const char* name = data_ptr->file_entries[idx][i].name.c_str();
        SDL_Surface *text_surf = TTF_RenderText_Solid(data_ptr->font, name, color);
        data_ptr->file_entries[idx][i].text_texture = SDL_CreateTextureFromSurface(renderer, text_surf);
        SDL_FreeSurface(text_surf);
        data_ptr->file_entries[idx][i].texture_rect.x = 32;
        data_ptr->file_entries[idx][i].texture_rect.y = (i*22);
        SDL_QueryTexture(data_ptr->file_entries[idx][i].text_texture, NULL, NULL, &(data_ptr->file_entries[idx][i].texture_rect.w), 
                         &(data_ptr->file_entries[idx][i].texture_rect.h));

        //creates and queues texture for the file image
        SDL_Surface *img_surf;
        if (data_ptr->file_entries[idx][i].type == "executable") {
            img_surf = IMG_Load("resrc/icon-exe.png");
        }else if(data_ptr->file_entries[idx][i].type == "directory") {
            img_surf = IMG_Load("resrc/icon-folder.png");
        }else if(data_ptr->file_entries[idx][i].type == "image") {
            img_surf = IMG_Load("resrc/icon-image.png");
        }else if(data_ptr->file_entries[idx][i].type == "video") {
            img_surf = IMG_Load("resrc/icon-video.png");
        }else if(data_ptr->file_entries[idx][i].type == "other") {
            img_surf = IMG_Load("resrc/icon-unknown.png");
        }else if(data_ptr->file_entries[idx][i].type == "code") {
            img_surf = IMG_Load("resrc/icon-gear.png");
        }
        data_ptr->file_entries[idx][i].icon_texture = SDL_CreateTextureFromSurface(renderer, img_surf);
        SDL_FreeSurface(img_surf);
        data_ptr->file_entries[idx][i].icon_rect.x = 10;
        data_ptr->file_entries[idx][i].icon_rect.y = i*22;
        data_ptr->file_entries[idx][i].icon_rect.w = 10;
        data_ptr->file_entries[idx][i].icon_rect.h = 10;
        SDL_QueryTexture(data_ptr->file_entries[idx][i].icon_texture, NULL, NULL, &(data_ptr->file_entries[idx][i].icon_rect.w), 
                         &(data_ptr->file_entries[idx][i].icon_rect.h));
        if (data_ptr->file_entries[idx][i].texture_rect.w > largest_width) {
            largest_width = data_ptr->file_entries[idx][i].texture_rect.w;
        }
    }
    data_ptr->largest_width = largest_width;
    //std::cout << largest_width + 32 << std::endl;
    if (data_ptr->rec_toggle == false) {
        for (int i=0; i<data_ptr->file_entries[idx].size(); i++) {
            if (!(data_ptr->file_entries[idx][i].type == "directory")) {
                std::stringstream ss;
                ss << data_ptr->file_entries[idx][i].size;
                std::string file_size = ss.str();
                file_size += " " + data_ptr->file_entries[idx][i].units;
                const char* fsize = file_size.c_str();
                SDL_Surface *size_text_surf = TTF_RenderText_Solid(data_ptr->font, fsize, color);
                data_ptr->file_entries[idx][i].size_text_texture = SDL_CreateTextureFromSurface(renderer, size_text_surf);
                SDL_FreeSurface(size_text_surf);
                data_ptr->file_entries[idx][i].size_texture_rect.x = 36+ data_ptr->largest_width;
                data_ptr->file_entries[idx][i].size_texture_rect.y = (i*22);
                SDL_QueryTexture(data_ptr->file_entries[idx][i].size_text_texture, NULL, NULL, &(data_ptr->file_entries[idx][i].size_texture_rect.w), 
                                &(data_ptr->file_entries[idx][i].size_texture_rect.h));
            }
        }
    }
    
}

void initRecursive(SDL_Renderer *renderer, AppData *data_ptr, std::string path, int idx, int x_offset) {
    // recursively draw all files and subdirectories
    std::string temp_path;
    for (int i=0; i<data_ptr->file_entries[idx].size(); i++) {

        //Creating element-----------------
        SDL_Color color = { 0, 0, 0 };
        const char* name = data_ptr->file_entries[idx][i].name.c_str();
        SDL_Surface *text_surf = TTF_RenderText_Solid(data_ptr->font, name, color);
        data_ptr->file_entries[idx][i].text_texture = SDL_CreateTextureFromSurface(renderer, text_surf);
        SDL_FreeSurface(text_surf);
        data_ptr->file_entries[idx][i].texture_rect.x = 32 + (15 * x_offset);
        data_ptr->file_entries[idx][i].texture_rect.y = data_ptr->last_y;

        SDL_QueryTexture(data_ptr->file_entries[idx][i].text_texture, NULL, NULL, &(data_ptr->file_entries[idx][i].texture_rect.w), 
                         &(data_ptr->file_entries[idx][i].texture_rect.h));

        //creates and queues texture for the file image
        SDL_Surface *img_surf;
        if (data_ptr->file_entries[idx][i].type == "executable") {
            img_surf = IMG_Load("resrc/icon-exe.png");
        }else if(data_ptr->file_entries[idx][i].type == "directory") {
            img_surf = IMG_Load("resrc/icon-folder.png");
        }else if(data_ptr->file_entries[idx][i].type == "image") {
            img_surf = IMG_Load("resrc/icon-image.png");
        }else if(data_ptr->file_entries[idx][i].type == "video") {
            img_surf = IMG_Load("resrc/icon-video.png");
        }else if(data_ptr->file_entries[idx][i].type == "other") {
            img_surf = IMG_Load("resrc/icon-unknown.png");
        }else if(data_ptr->file_entries[idx][i].type == "code") {
            img_surf = IMG_Load("resrc/icon-gear.png");
        }
        data_ptr->file_entries[idx][i].icon_texture = SDL_CreateTextureFromSurface(renderer, img_surf);
        SDL_FreeSurface(img_surf);
        data_ptr->file_entries[idx][i].icon_rect.x = 10 + (15 * x_offset);
        data_ptr->file_entries[idx][i].icon_rect.y = data_ptr->last_y;
        data_ptr->last_y += 25;

        data_ptr->file_entries[idx][i].icon_rect.w = 10;
        data_ptr->file_entries[idx][i].icon_rect.h = 10;
        SDL_QueryTexture(data_ptr->file_entries[idx][i].icon_texture, NULL, NULL, &(data_ptr->file_entries[idx][i].icon_rect.w), 
                         &(data_ptr->file_entries[idx][i].icon_rect.h));
        //Creating element-----------------


        if (data_ptr->file_entries[idx][i].type == "directory" && data_ptr->file_entries[idx][i].name != "..") {
            temp_path = path + "/" + data_ptr->file_entries[idx][i].name;
            char *cstr = new char[temp_path.length() + 1];
            strcpy(cstr, temp_path.c_str());
            initializeFiles(data_ptr, cstr);
            delete [] cstr;
            initRecursive(renderer, data_ptr, temp_path, data_ptr->file_entries.size() - 1, x_offset + 1);
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

    //Render Recursive toggle
    if (data_ptr->rec_toggle) {
        SDL_RenderCopy(renderer, data_ptr->rec_g_img_texture, NULL, &(data_ptr->rec_g_img_rect));
    }else {
        SDL_RenderCopy(renderer, data_ptr->rec_r_img_texture, NULL, &(data_ptr->rec_r_img_rect));
    }
    
    SDL_RenderCopy(renderer, data_ptr->rec_text_texture, NULL, &(data_ptr->rec_text_rect));
    
    // TODO: draw!
    for (int i=0; i<data_ptr->file_entries.size(); i++) {
        for (int j=0; j<data_ptr->file_entries[i].size(); j++) {
            SDL_RenderCopy(renderer, data_ptr->file_entries[i][j].icon_texture, NULL, &(data_ptr->file_entries[i][j].icon_rect));
            SDL_RenderCopy(renderer, data_ptr->file_entries[i][j].text_texture, NULL, &(data_ptr->file_entries[i][j].texture_rect));
            //size_text_texture is the problem
            if (!(data_ptr->file_entries[i][j].type == "directory") && data_ptr->rec_toggle == false) {
                SDL_RenderCopy(renderer, data_ptr->file_entries[i][j].size_text_texture, NULL, &(data_ptr->file_entries[i][j].size_texture_rect));
            }
        }
    }

    // show rendered frame
    SDL_RenderPresent(renderer);
}

void initializeRecToggle(SDL_Renderer *renderer, AppData *data_ptr) {
    SDL_Color color = { 0, 0, 0 };
    data_ptr->rec_toggle = false;
    data_ptr->font = TTF_OpenFont("resrc/OpenSans-Regular.ttf", 14);

    //Creating red and green buttons
    SDL_Surface *rec_r_img_surf = IMG_Load("resrc/red.png");
    SDL_Surface *rec_g_img_surf = IMG_Load("resrc/green.png");
    SDL_Surface *rec_text_surf = TTF_RenderText_Solid(data_ptr->font, "Click to toggle recursive mode.", color);
    
    data_ptr->rec_r_img_texture = SDL_CreateTextureFromSurface(renderer, rec_r_img_surf);
    data_ptr->rec_g_img_texture = SDL_CreateTextureFromSurface(renderer, rec_g_img_surf);
    data_ptr->rec_text_texture = SDL_CreateTextureFromSurface(renderer, rec_text_surf);

    SDL_FreeSurface(rec_r_img_surf);
    SDL_FreeSurface(rec_g_img_surf);
    SDL_FreeSurface(rec_text_surf);

    data_ptr->rec_r_img_rect.x = 776;
    data_ptr->rec_r_img_rect.y = 2;
    data_ptr->rec_r_img_rect.w = 10;
    data_ptr->rec_r_img_rect.h = 10;
    data_ptr->rec_g_img_rect.x = 776;
    data_ptr->rec_g_img_rect.y = 2;
    data_ptr->rec_g_img_rect.w = 10;
    data_ptr->rec_g_img_rect.h = 10;
    data_ptr->rec_text_rect.x = 570;
    data_ptr->rec_text_rect.y = 2;

    SDL_QueryTexture(data_ptr->rec_r_img_texture, NULL, NULL, &(data_ptr->rec_r_img_rect.w), &(data_ptr->rec_r_img_rect.h));
    SDL_QueryTexture(data_ptr->rec_g_img_texture, NULL, NULL, &(data_ptr->rec_g_img_rect.w), &(data_ptr->rec_g_img_rect.h));
    SDL_QueryTexture(data_ptr->rec_text_texture, NULL, NULL, &(data_ptr->rec_text_rect.w), &(data_ptr->rec_text_rect.h));
}
