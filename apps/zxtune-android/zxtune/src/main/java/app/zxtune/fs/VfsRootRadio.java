package app.zxtune.fs;

import android.content.Context;
import android.net.Uri;

import androidx.annotation.Nullable;

import java.io.IOException;
import java.util.ArrayList;

import app.zxtune.R;

@Icon(R.drawable.ic_browser_vfs_radio)
final class VfsRootRadio extends StubObject implements VfsRoot {

  private final Uri URI = Uri.fromParts("radio", "", "");

  private final Context ctx;
  private final ArrayList<VfsDir> children = new ArrayList<>();

  VfsRootRadio(Context ctx) {
    this.ctx = ctx;
  }

  @Nullable
  @Override
  public VfsObject resolve(Uri uri) {
    if (URI.equals(uri)) {
      return this;
    } else {
      return null;
    }
  }

  @Override
  public void enumerate(Visitor visitor) throws IOException {
    defferredFill();
    for (VfsDir child : children) {
      visitor.onDir(child);
    }
  }

  @Override
  public Uri getUri() {
    return URI;
  }

  @Override
  public String getName() {
    return ctx.getString(R.string.vfs_radio_root_name);
  }

  @Override
  public String getDescription() {
    return ctx.getString(R.string.vfs_radio_root_description);
  }

  @Nullable
  @Override
  public VfsObject getParent() {
    return null;
  }

  private void defferredFill() throws IOException {
    if (children.isEmpty()) {
      children.add(new EntryModarchive());
    }
  }

  // TODO: get rid of wrappers after browser rework
  @Icon(R.drawable.ic_browser_vfs_modarchive)
  private class EntryModarchive extends StubObject implements VfsDir {

    private final VfsDir delegate;

    EntryModarchive() throws IOException {
      this.delegate = (VfsDir) Vfs.resolve(Uri.parse("modarchive:/Random"));
    }

    @Override
    public Uri getUri() {
      return delegate.getUri();
    }

    @Override
    public String getName() {
      return ctx.getString(R.string.vfs_modarchive_random_name);
    }

    @Override
    public String getDescription() {
      return ctx.getString(R.string.vfs_modarchive_root_name);
    }

    @Override
    public VfsObject getParent() {
      // not required really
      return delegate.getParent();
    }

    @Override
    public Object getExtension(String id) {
      return delegate.getExtension(id);
    }

    @Override
    public void enumerate(Visitor visitor) throws IOException {
      delegate.enumerate(visitor);
    }
  }
}
