#include "graphics.h"
void drawString(char *string, int x, int y, int ishl)
{
    SDL_Color color = {255, 255, 255};
    SDL_Surface *surface = NULL;
    surface = TTF_RenderText_Solid(font, string, color);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect dstrect = {x, y, 0, 0};
    dstrect.h = surface->h;
    dstrect.w = surface->w;

    if (ishl)
    {
        SDL_Rect dest;
        SDL_Surface *rect_surface = SDL_GetWindowSurface(window);
        SDL_FillRect(rect_surface, NULL, SDL_MapRGB(rect_surface->format, 196, 36, 36));
        SDL_Texture *rect_texture = SDL_CreateTextureFromSurface(renderer, rect_surface);

        dest.x = dstrect.x;
        dest.y = dstrect.y + dstrect.h + 1;
        dest.w = 200;
        dest.h = 2;
        SDL_RenderCopy(renderer, rect_texture, NULL, &dest);
        SDL_FreeSurface(rect_surface);
        SDL_DestroyTexture(rect_texture);
    }

    SDL_FreeSurface(surface);
    SDL_RenderCopy(renderer, texture, NULL, &dstrect);
    SDL_DestroyTexture(texture);
}

void drawStringln(char *string, int x, int isSelected)
{
    SDL_Color color = {255, 255, 255};
    SDL_Surface *surface = NULL;
    surface = TTF_RenderText_Solid(font, string, color);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect dstrect = {x, LINE_HIGHT * line_count, 0, 0};
    line_count++;
    dstrect.h = surface->h;
    dstrect.w = surface->w;
    if (isSelected)
    {
        SDL_Rect dest;
        SDL_Surface *rect_surface = SDL_GetWindowSurface(window);
        SDL_FillRect(rect_surface, NULL, SDL_MapRGB(rect_surface->format, 196, 36, 36));
        SDL_Texture *rect_texture = SDL_CreateTextureFromSurface(renderer, rect_surface);

        dest.x = dstrect.x;
        dest.y = dstrect.y;
        dest.w = 150;
        dest.h = surface->h;
        SDL_RenderCopy(renderer, rect_texture, NULL, &dest);
        SDL_FreeSurface(rect_surface);
        SDL_DestroyTexture(rect_texture);
    }

    SDL_FreeSurface(surface);
    SDL_RenderCopy(renderer, texture, NULL, &dstrect);
    SDL_DestroyTexture(texture);
}