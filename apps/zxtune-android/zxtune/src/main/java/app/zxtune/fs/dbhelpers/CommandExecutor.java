package app.zxtune.fs.dbhelpers;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.util.concurrent.TimeUnit;

import app.zxtune.Analytics;
import app.zxtune.Log;
import app.zxtune.TimeStamp;
import app.zxtune.fs.http.HttpObject;
import app.zxtune.io.Io;
import app.zxtune.io.TransactionalOutputStream;

public class CommandExecutor {

  private static final String TAG = CommandExecutor.class.getName();

  private final String id;

  public CommandExecutor(String id) {
    this.id = id;
  }

  public final void executeQuery(String scope, QueryCommand cmd) throws IOException {
    if (cmd.getLifetime().isExpired()) {
      refreshAndQuery(scope, cmd);
    } else {
      cmd.queryFromCache();
      Analytics.sendVfsCacheEvent(id, scope);
    }
  }

  private void refreshAndQuery(String scope, QueryCommand cmd) throws IOException {
    try {
      executeRefresh(scope, cmd);
      cmd.queryFromCache();
    } catch (IOException e) {
      if (cmd.queryFromCache()) {
        Analytics.sendVfsCacheEvent(id, scope);
      } else {
        throw e;
      }
    }
  }

  public final void executeRefresh(String scope, QueryCommand cmd) throws IOException {
    final Transaction transaction = cmd.startTransaction();
    try {
      cmd.updateCache();
      cmd.getLifetime().update();
      transaction.succeed();
      Analytics.sendVfsRemoteEvent(id, scope);
    } finally {
      transaction.finish();
    }
  }

  public final <T> T executeFetchCommand(String scope, FetchCommand<T> cmd) throws IOException {
    final T cached = cmd.fetchFromCache();
    if (cached != null) {
      Analytics.sendVfsCacheEvent(id, scope);
      return cached;
    }
    final T remote = cmd.updateCache();
    Analytics.sendVfsRemoteEvent(id, scope);
    return remote;
  }

  public final ByteBuffer executeDownloadCommand(DownloadCommand cmd) throws IOException {
    final String scope = "file";
    ByteBuffer result = null;
    try {
      final File cache = cmd.getCache();
      final boolean isEmpty = cache.length() == 0;//also if not exists
      if (isEmpty || needUpdate(cache)) {
        try {
          final HttpObject remote = cmd.getRemote();
          if (isEmpty || needUpdate(cache, remote)) {
            Log.d(TAG,"Download %s to %s", remote.getUri(), cache.getAbsolutePath());
            download(remote.getInput(), cache);
          } else {
            Io.touch(cache);
          }
          result = Io.readFrom(cache);
        } catch (IOException e) {
          Log.w(TAG, e, "Failed to update cache");
        }
      }
      if (result == null && cache.canRead()) {
        result = Io.readFrom(cache);
        Analytics.sendVfsCacheEvent(id, scope);
      }
    } catch (IOException e) {
      Log.w(TAG, e, "Failed to load from cache");
    }
    if (result == null) {
      result = Io.readFrom(cmd.getRemote().getInput());
      Analytics.sendVfsRemoteEvent(id, scope);
    }
    return result;
  }

  private static boolean needUpdate(File cache) {
    final long CACHE_CHECK_PERIOD_MS = 24 * 60 * 60 * 1000;
    final long lastModified = cache.lastModified();
    if (lastModified == 0) {
      return false;//not supported
    }
    return lastModified + CACHE_CHECK_PERIOD_MS < System.currentTimeMillis();
  }

  private static boolean needUpdate(File cache, HttpObject remote) {
    return remoteUpdated(cache, remote) || sizeChanged(cache, remote);
  }

  private static boolean remoteUpdated(File cache, HttpObject remote) {
    final long localLastModified = cache.lastModified();
    final TimeStamp remoteLastModified = remote.getLastModified();
    if (localLastModified != 0 && remoteLastModified != null && remoteLastModified.convertTo(TimeUnit.MILLISECONDS) > localLastModified) {
      Log.d(TAG, "Update outdated file: %d -> %d", localLastModified, remoteLastModified.convertTo(TimeUnit.MILLISECONDS));
      return true;
    } else {
      return false;
    }
  }

  private static boolean sizeChanged(File cache, HttpObject remote) {
    final long localSize = cache.length();
    final Long remoteSize = remote.getContentLength();
    if (remoteSize != null && remoteSize != localSize) {
      Log.d(TAG, "Update changed file: %d -> %d", localSize, remoteSize.longValue());
      return true;
    }
    else {
      return false;
    }
  }

  private void download(InputStream input, File cache) throws IOException {
    try {
      final TransactionalOutputStream output = new TransactionalOutputStream(cache);
      try {
        Io.copy(input, output);
        output.flush();
        Analytics.sendVfsRemoteEvent(id, "file");
      } finally {
        output.close();
      }
    } finally {
      input.close();
    }
  }
}
