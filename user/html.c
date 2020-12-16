#define HTML_HEADER \
	"<!DOCTYPE html><html>" \
	"<head><title>ESP8266 Firstboot config</title></head><body>\r\n" 

#define HTML_FOOTER "\r\n</body></html>\r\n"

#define HTML_FORM \
    HTML_HEADER \
    "<form action=\"/\" method=\"post\"" \
        "enctype=\"application/x-www-form-urlencoded\">" \
        "Choose Your favourite color" \
        "<input type=\"color\" name=\"c\" value=\"%s\"/>" \
	    "<input type=\"submit\"/>" \
    "</form>"\
    HTML_FOOTER

