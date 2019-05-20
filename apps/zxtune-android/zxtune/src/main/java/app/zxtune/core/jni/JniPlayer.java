package app.zxtune.core.jni;

import android.support.annotation.NonNull;
import app.zxtune.core.ModuleAttributes;
import app.zxtune.core.Player;

public final class JniPlayer implements Player {

  @SuppressWarnings({"FieldCanBeLocal", "unused"})
  private final int handle;

  JniPlayer(int handle) {
    this.handle = handle;
    JniGC.register(this, handle, getProperty(ModuleAttributes.TYPE, "Unknown"));
  }

  static native void close(int handle);
  static native int getPlaybackPerformance(int player);

  @Override
  public native boolean render(@NonNull short[] result);

  @Override
  public native int analyze(@NonNull byte levels[]);

  @Override
  public native int getPosition();

  @Override
  public native void setPosition(int pos);

  @Override
  public native long getProperty(@NonNull String name, long defVal);

  @NonNull
  @Override
  public native String getProperty(@NonNull String name, @NonNull String defVal);

  @Override
  public native void setProperty(@NonNull String name, long val);

  @Override
  public native void setProperty(@NonNull String name, @NonNull String val);
}