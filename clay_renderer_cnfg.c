//#define CNFA_IMPLEMENTATION
#define CNFG_IMPLEMENTATION
#define CNFG3D
#include "CNFGAndroid.h"
#include "CNFG.h"
#include "stdint.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#ifdef CLAY_OVERFLOW_TRAP
#include "signal.h"
#endif

#define CLAY_COLOR_TO_CNFG_COLOR(color) (((unsigned char) roundf(color.r) & 0xff) << 24) + (((unsigned char) roundf(color.g) & 0xff) << 16) + (((unsigned char) roundf(color.b) & 0xff) << 8) + ((unsigned char) roundf(color.a) & 0xff)

typedef struct Image {
    int width;
    int height;
    uint32_t* data;
} Image;

static inline void CNFG_DrawRectangle(short x0, short y0, short width, short height, uint32_t color) {
    CNFGColor( color );
    CNFGTackRectangle( x0, y0, x0 + width, y0 + height );
}

static inline Clay_Dimensions CNFG_MeasureText(Clay_String *text, Clay_TextElementConfig *config) {
    // Measure string size for Font
    Clay_Dimensions textSize = { 0 };

    //TODO use config->fontSize and the DPI to convert to pixels
    float scale = 3;
    float widthLine = 0;

    textSize.height = 6 * scale;
    
    for (int i = 0; i < text->length; ++i)
    {
        switch (text->chars[i])
        {
        case 9: // tab
			widthLine += 12 * scale;
			break;
		case 10: // linefeed
			textSize.width = widthLine > textSize.width ? widthLine : textSize.width;
			textSize.height += 6 * scale;
			widthLine = 0;
			break;
        default:
            widthLine += 3 * scale;
            break;
        }
    }

    textSize.width = widthLine > textSize.width ? widthLine : textSize.width;

    return textSize;
}

void Clay_CNFG_Initialize(int width, int height, const char *title, unsigned int flags) {
    //TODO
}

void Clay_CNFG_Render(short width, short height, Clay_RenderCommandArray renderCommands)
{
    for (int j = 0; j < renderCommands.length; j++)
    {
        Clay_RenderCommand *renderCommand = Clay_RenderCommandArray_Get(&renderCommands, j);
        Clay_BoundingBox boundingBox = renderCommand->boundingBox;
        switch (renderCommand->commandType)
        {
            case CLAY_RENDER_COMMAND_TYPE_TEXT: {
                // CNLR uses standard C strings so isn't compatible with cheap slices, we need to clone the string to append null terminator
                Clay_String text = renderCommand->text;
                char *cloned = (char *)malloc(text.length + 1);
                memcpy(cloned, text.chars, text.length);
                cloned[text.length] = '\0';
                
                //TODO use renderCommand->config.textElementConfig->fontSize and the DPI to convert to pixels
                short pixelSize = 3;
                
                CNFGPenX = boundingBox.x; CNFGPenY = boundingBox.y;
                CNFGColor( CLAY_COLOR_TO_CNFG_COLOR(renderCommand->config.textElementConfig->textColor) );
                CNFGDrawText(cloned, pixelSize);
                
                free(cloned);
                break;
            }
            case CLAY_RENDER_COMMAND_TYPE_IMAGE: {
                //TODO generate new image
                Image* image = renderCommand->config.imageElementConfig->imageData;
                
                //Copy the image to resize it
                uint32_t *resized = (uint32_t *)malloc(boundingBox.width * boundingBox.height * sizeof(uint32_t));
                for (int x = 0; x < boundingBox.width; x++) {
                    for (int y = 0; y < boundingBox.height; y++) {
                        // From https://courses.cs.vt.edu/~masc1044/L17-Rotation/ScalingNN.html
                        int sourceX = (int) (roundf( x / boundingBox.width * image->width));
                        int sourceY = (int) (roundf( y / boundingBox.height * image->height));
                        resized[y * (int)(roundf(boundingBox.width)) + x] = image->data[sourceY * image->width + sourceX];
                    }
                }
            
                CNFGBlitImage( resized, boundingBox.x, boundingBox.y, boundingBox.width, boundingBox.height);
                
                free(resized);
                break;
            }
            case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START: {
                //glScissor((int)roundf(boundingBox.x), height - (int)roundf(boundingBox.y + boundingBox.height), (int)roundf(boundingBox.width), (int)roundf(boundingBox.height));
                //glEnable(GL_SCISSOR_TEST);
                break;
            }
            case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END: {
                //glDisable(GL_SCISSOR_TEST);
                break;
            }
            case CLAY_RENDER_COMMAND_TYPE_RECTANGLE: {
                Clay_RectangleElementConfig *config = renderCommand->config.rectangleElementConfig;
                
                CNFG_DrawRectangle(boundingBox.x, boundingBox.y, boundingBox.width, boundingBox.height, CLAY_COLOR_TO_CNFG_COLOR(config->color));

		        if (config->cornerRadius.topLeft > 0) {
                    //TODO
                }
                break;
            }
            case CLAY_RENDER_COMMAND_TYPE_BORDER: {
                Clay_BorderElementConfig *config = renderCommand->config.borderElementConfig;
                // Left border
                if (config->left.width > 0) {
                    CNFG_DrawRectangle((int)roundf(boundingBox.x), (int)roundf(boundingBox.y + config->cornerRadius.topLeft), (int)config->left.width, (int)roundf(boundingBox.height - config->cornerRadius.topLeft - config->cornerRadius.bottomLeft), CLAY_COLOR_TO_CNFG_COLOR(config->left.color));
                }
                // Right border
                if (config->right.width > 0) {
                    CNFG_DrawRectangle((int)roundf(boundingBox.x + boundingBox.width - config->right.width), (int)roundf(boundingBox.y + config->cornerRadius.topRight), (int)config->right.width, (int)roundf(boundingBox.height - config->cornerRadius.topRight - config->cornerRadius.bottomRight), CLAY_COLOR_TO_CNFG_COLOR(config->right.color));
                }
                // Top border
                if (config->top.width > 0) {
                    CNFG_DrawRectangle((int)roundf(boundingBox.x + config->cornerRadius.topLeft), (int)roundf(boundingBox.y), (int)roundf(boundingBox.width - config->cornerRadius.topLeft - config->cornerRadius.topRight), (int)config->top.width, CLAY_COLOR_TO_CNFG_COLOR(config->top.color));
                }
                // Bottom border
                if (config->bottom.width > 0) {
                    CNFG_DrawRectangle((int)roundf(boundingBox.x + config->cornerRadius.bottomLeft), (int)roundf(boundingBox.y + boundingBox.height - config->bottom.width), (int)roundf(boundingBox.width - config->cornerRadius.bottomLeft - config->cornerRadius.bottomRight), (int)config->bottom.width, CLAY_COLOR_TO_CNFG_COLOR(config->bottom.color));
                }
                if (config->cornerRadius.topLeft > 0) {
                    //TODO DrawRing((Vector2) { roundf(boundingBox.x + config->cornerRadius.topLeft), roundf(boundingBox.y + config->cornerRadius.topLeft) }, roundf(config->cornerRadius.topLeft - config->top.width), config->cornerRadius.topLeft, 180, 270, 10, CLAY_COLOR_TO_RAYLIB_COLOR(config->top.color));
                }
                if (config->cornerRadius.topRight > 0) {
                    //TODO DrawRing((Vector2) { roundf(boundingBox.x + boundingBox.width - config->cornerRadius.topRight), roundf(boundingBox.y + config->cornerRadius.topRight) }, roundf(config->cornerRadius.topRight - config->top.width), config->cornerRadius.topRight, 270, 360, 10, CLAY_COLOR_TO_RAYLIB_COLOR(config->top.color));
                }
                if (config->cornerRadius.bottomLeft > 0) {
                    //TODO DrawRing((Vector2) { roundf(boundingBox.x + config->cornerRadius.bottomLeft), roundf(boundingBox.y + boundingBox.height - config->cornerRadius.bottomLeft) }, roundf(config->cornerRadius.bottomLeft - config->top.width), config->cornerRadius.bottomLeft, 90, 180, 10, CLAY_COLOR_TO_RAYLIB_COLOR(config->bottom.color));
                }
                if (config->cornerRadius.bottomRight > 0) {
                    //TODO DrawRing((Vector2) { roundf(boundingBox.x + boundingBox.width - config->cornerRadius.bottomRight), roundf(boundingBox.y + boundingBox.height - config->cornerRadius.bottomRight) }, roundf(config->cornerRadius.bottomRight - config->bottom.width), config->cornerRadius.bottomRight, 0.1, 90, 10, CLAY_COLOR_TO_RAYLIB_COLOR(config->bottom.color));
                }
                break;
            }
            case CLAY_RENDER_COMMAND_TYPE_CUSTOM: {
                //TODO
                /* 
                CustomLayoutElement *customElement = (CustomLayoutElement *)renderCommand->config.customElementConfig->customData;
                if (!customElement) continue;
                switch (customElement->type) {
                    case CUSTOM_LAYOUT_ELEMENT_TYPE_3D_MODEL: {
                        Clay_BoundingBox rootBox = renderCommands.internalArray[0].boundingBox;
                        float scaleValue = CLAY__MIN(CLAY__MIN(1, 768 / rootBox.height) * CLAY__MAX(1, rootBox.width / 1024), 1.5f);
                        Ray positionRay = GetScreenToWorldPointWithZDistance((Vector2) { renderCommand->boundingBox.x + renderCommand->boundingBox.width / 2, renderCommand->boundingBox.y + (renderCommand->boundingBox.height / 2) + 20 }, Raylib_camera, (int)roundf(rootBox.width), (int)roundf(rootBox.height), 140);
                        BeginMode3D(Raylib_camera);
                            DrawModel(customElement->model.model, positionRay.position, customElement->model.scale * scaleValue, WHITE);        // Draw 3d model with texture
                        EndMode3D();
                        break;
                    }
                    default: break;
                }
                */
                break;
            }
            default: {
                printf("Error: unhandled render command.");
                #ifdef CLAY_OVERFLOW_TRAP
                raise(SIGTRAP);
                #endif
            }
        }
    }
}

