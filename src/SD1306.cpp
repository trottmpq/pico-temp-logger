#include "SSD1306.hpp"

/*!
    @brief  Constructor for I2C-interfaced OLED display.
    @param  DevAddr
            Device i2c address shifted one to the left.
    @param  width
            Display width.
    @param  height
            Display height.
    @param  i2c
            Pointer to an existing i2c instance.
    @return SSD1306 object.
*/
SSD1306::SSD1306(uint16_t const DevAddr, size Size, i2c_inst_t * i2c) : DevAddr(DevAddr), width(width), height(height), i2c(i2c), Size(Size)
{
	if(this->Size == size::W64xH32)
	{
		this->width = 64;
		this->height = 32;
	} else if (this->Size == size::W128xH64)
	{
		this->width = 128;
		this->height = 64;
	} else
	{
		this->width = 128;
		this->height = 32;
	}

	this->bufferlen = this->width * (this->height / 8);
	this->buffer = new unsigned char[this->bufferlen];

	this->displayON(false);

	// this->sendCommand(SSD1306_SETLOWCOLUMN);
	// this->sendCommand(SSD1306_SETHIGHCOLUMN);
    
	/* memory mapping */
	this->sendCommand(SSD1306_MEMORYMODE); // set memory address mode 0 = horizontal, 1 = vertical, 2 = page
	this->sendCommand(0x00); // horizontal addressing mode

    /* resolution and layout */
	this->sendCommand(SSD1306_SETSTARTLINE); // set display start line to 0
	this->sendCommand(SSD1306_SEGREMAP | 0x01); // set segment re-map, column address 127 is mapped to SEG0

	this->sendCommand(SSD1306_SETMULTIPLEX); // set multiplex ratio
	this->sendCommand(height - 1); // our display is only 32 pixels high

	this->sendCommand(SSD1306_COMSCANINC | 0x08); // set COM (common) output scan direction. Scan from bottom up, COM[N-1] to COM0
	
	this->sendCommand(SSD1306_SETDISPLAYOFFSET); // set display offset
	this->sendCommand(0x00); // no offset

	this->sendCommand(SSD1306_SETCOMPINS); // set COM (common) pins hardware configuration. Board specific magic number. 
                                           // 0x02 Works for 128x32, 0x12 Possibly works for 128x64. Other options 0x22, 0x32
	this->sendCommand(0x02); // manufacturer magic number
	
	this->sendCommand(SSD1306_SETDISPLAYCLOCKDIV); // set display clock divide ratio
	this->sendCommand(0x80); // div ratio of 1, standard freq

	this->sendCommand(SSD1306_SETPRECHARGE); // set pre-charge period
	this->sendCommand(0xF1); // Vcc internally generated on our board

	this->sendCommand(SSD1306_SETVCOMDETECT); // set VCOMH deselect level
	this->sendCommand(0x40);

	this->setContrast(0xFF); // set contrast control

	this->sendCommand(SSD1306_DISPLAYALLON_RESUME); // set entire display on to follow RAM content

	this->sendCommand(SSD1306_NORMALDISPLAY); // set normal (not inverted) display

	this->sendCommand(SSD1306_CHARGEPUMP); // set charge pump
	this->sendCommand(0x14); // Vcc internally generated on our board
	
	this->sendCommand(SSD1306_SETSCROLL | 0x00); // deactivate horizontal scrolling if set. This is necessary as memory writes will corrupt if scrolling was enabled

	this->displayON(1);

	    // Initialize render area for entire frame (SSD1306_WIDTH pixels by SSD1306_NUM_PAGES pages)
    this->frame_area = {
        start_col: 0,
        end_col : this->width - 1,
        start_page : 0,
        end_page : (this->height / 8) - 1
        };

    this->calculateRenderAreaBuffLen(&frame_area);

	// this->rotateDisplay(1);
	// this->sendCommand(0x3F);
	// this->clear();
	// this->display();
}


/*!
    @brief  Deconstructor for I2C-interfaced OLED display.
*/
SSD1306::~SSD1306() 
{
	delete this->buffer;
}


/*!
 * @brief Send command to display.
 *
 */
void SSD1306::sendCommand(uint8_t command)
{	
	uint8_t mess[2] = {0x00, command};
	i2c_write_blocking(this->i2c, this->DevAddr, mess, 2, false);
}


/*!
 * @brief Invert colors.
 *
 */
void SSD1306::invertColors(uint8_t Invert)
{
	this->sendCommand(Invert ? SSD1306_INVERTDISPLAY : SSD1306_NORMALDISPLAY);
}


/*!
 * @brief Rotate display.
 *
 */
void SSD1306::rotateDisplay(uint8_t Rotate)
{
	if(Rotate > 1) Rotate = 1;

	this->sendCommand(0xA0 | (0x01 & Rotate));  // Set Segment Re-Map Default
							// 0xA0 (0x00) => column Address 0 mapped to 127
                			// 0xA1 (0x01) => Column Address 127 mapped to 0

	this->sendCommand(0xC0 | (0x08 & (Rotate<<3)));  // Set COM Output Scan Direction
							// 0xC0	(0x00) => normal mode (RESET) Scan from COM0 to COM[N-1];Where N is the Multiplex ratio.
							// 0xC8	(0xC8) => remapped mode. Scan from COM[N-1] to COM0;;Where N is the Multiplex ratio.
}


/*!
 * @brief Turn on display.
 * 0 – Turn OFF
 * 1 – Turn ON
 */
void SSD1306::displayON(uint8_t On)
{
	this->sendCommand(On ? SSD1306_DISPLAYON : SSD1306_DISPLAYOFF);
}


/*!
 * @brief Set contrast.
 *
 */
void SSD1306::setContrast(uint8_t Contrast)
{
	this->sendCommand(SSD1306_SETCONTRAST);
	this->sendCommand(Contrast);
}


/*!
 * @brief Draw pixel in the buffer.
 * @param x position from the left edge (0, MAX WIDTH)
 * @param y position from the top edge (0, MAX HEIGHT)
 * @param color colors::BLACK, colors::WHITE or colors::INVERSE
 */
void SSD1306::drawPixel(int16_t x, int16_t y, colors Color)
{
	assert(x >= 0 && x < this->width && y >=0 && y < this->height);

    // The calculation to determine the correct bit to set depends on which address
    // mode we are in. This code assumes horizontal

    // The video ram on the SSD1306 is split up in to 8 rows, one bit per pixel.
    // Each row is 128 long by 8 pixels high, each byte vertically arranged, so byte 0 is x=0, y=0->7,
    // byte 1 is x = 1, y=0->7 etc

    // This code could be optimised, but is like this for clarity. The compiler
    // should do a half decent job optimising it anyway.

	const int BytesPerRow = this->width ; // x pixels, 1bpp, but each row is 8 pixel high, so (x / 8) * 8

    int byte_idx = (y / 8) * BytesPerRow + x;
    uint8_t byte = this->buffer[byte_idx];

	switch(Color)
	{
		case colors::WHITE:
			byte |=  1 << (y % 8); 
			break;
		case colors::BLACK:
			byte &= ~(1 << (y % 8));
			break;
		case colors::INVERSE: 
			byte ^= (1 << (y % 8));
			break;
	}
	this->buffer[byte_idx] = byte;
}


/*!
 * @brief Clear the buffer.
 * @param color colors::BLACK, colors::WHITE or colors::INVERSE
 */
void SSD1306::clear(colors Color)
{
	switch (Color)
	{
		case colors::WHITE:
			memset(buffer, 0xFF, this->bufferlen);
			break;
		case colors::BLACK:
			memset(buffer, 0x00, this->bufferlen);
			break;
	}
}


/*!
 * @brief Send buffer to OLED GCRAM.
 * @param data (Optional) Pointer to data array.
 */
void SSD1306::display(unsigned char *data)
{
	if(data == nullptr) data = this->buffer;
	this->sendCommand(0x21); // SSD1306_SET_COL_ADDR
	this->sendCommand(frame_area.start_col);
	this->sendCommand(frame_area.end_col);
	this->sendCommand(0x22);// SSD1306_SET_PAGE_ADDR
	this->sendCommand(frame_area.start_page);
	this->sendCommand(frame_area.end_page);
	this->sendData(data, this->bufferlen);
}


void SSD1306::sendData(uint8_t* buffer, size_t buff_size)
{
	unsigned char mess[buff_size+1];

	mess[0] = 0x40;
	memcpy(mess+1, buffer, buff_size);

	i2c_write_blocking(this->i2c, this->DevAddr, mess, buff_size+1, false);
}


void SSD1306::calculateRenderAreaBuffLen(struct render_area *area) {
    // calculate how long the flattened buffer will be for a render area
    area->buflen = (area->end_col - area->start_col + 1) * (area->end_page - area->start_page + 1);
}

/*!
 * @brief Return display height.
 * @return display height
 */
uint8_t SSD1306::getHeight()
{
	if(this->Size == size::W128xH64) return 64;
	else return 32;
}

/*!
 * @brief Return display width.
 * @return display width
 */
uint8_t SSD1306::getWidth()
{
	if(this->Size == size::W64xH32) return 64;
	else return 128;
}