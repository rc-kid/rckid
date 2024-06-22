#pragma once

namespace rckid {

    /** Telegram Messenger

        

        API points

        - `/getMe` - returns information about the bot used.
        - `/setMyName?name=` - sets the display name for the bot
        - `/getUpdates?offset=X&limit=Y` - returns the updates, offset must be last processed offset + 1 to commit. Updates will only last for 24 hrs
        - `/sendMessage?chat_id=&text=` sends text message to given chat
        - `/sendPhoto?chat_id=&photo=` sends previously uploaded photo (png/jpeg)
        - `/sendVoice?chat_id=&voice=` sends previously uploaded voice (ogg/opus)
        - `/leaveChat?chat_id=` to leave given chat (when contact is deleted?)
        - `/getFile?file_id=` gets information about a file and prepares it for download

        Downloading files

        - getUpdates will have file id in the updated messages
        - getFile with that id returns JSON file object, which contains amongst other things the file path
        - the file can then be downloaded directly from the given address:

        https://api.telegram.org/file/bot<TOKEN>/<FILE_PATH>

        Uploading files

        - files are uploaded as part of the message sent to the server (i.e. sendPhoto or sendVoice, etc.)

        Notes

        - it looks that when videos are sent, there is jpg o png thumb provided as well, which can be used to show things 

     */
    class Messenger {

    }; // rckid::Messenger


} // namespace rckid