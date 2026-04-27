package @TD_ANDROID_PACKAGE@;

import org.libsdl.app.SDLActivity;

public class @TD_ANDROID_ACTIVITY@ extends SDLActivity {
    @Override
    protected String[] getLibraries() {
        return new String[] {
            "SDL3",
            "tiberian-dawn"
        };
    }

    @Override
    protected String[] getArguments() {
        return new String[] {
            "-gamedata",
            "./"
        };
    }
}
