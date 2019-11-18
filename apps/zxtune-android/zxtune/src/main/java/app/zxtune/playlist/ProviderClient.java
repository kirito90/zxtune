package app.zxtune.playlist;

import android.content.ContentResolver;
import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import androidx.annotation.Nullable;
import androidx.loader.content.CursorLoader;
import androidx.loader.content.Loader;

import app.zxtune.analytics.Analytics;

public final class ProviderClient {

  //should be name-compatible with Database
  public enum SortBy {
    title,
    author,
    duration
  }

  public enum SortOrder {
    asc,
    desc
  }

  private final ContentResolver resolver;

  public ProviderClient(Context ctx) {
    this.resolver = ctx.getContentResolver();
  }

  public static Uri createUri(long id) {
    return PlaylistQuery.uriFor(id);
  }

  @Nullable
  public static Long findId(Uri uri) {
    return ContentResolver.SCHEME_CONTENT.equals(uri.getScheme())
        ? PlaylistQuery.idOf(uri)
               : null;
  }

  public final void addItem(Item item) {
    resolver.insert(PlaylistQuery.ALL, item.toContentValues());
  }

  //TODO: remove
  public final void updatePlaybackStatus(long id, boolean isPlaying) {
    final Uri uri = PlaylistQuery.uriFor(id);
    resolver.update(uri, new ItemState(isPlaying).toContentValues(), null, null);
    resolver.notifyChange(uri, null);
  }

  public final void notifyChanges() {
    resolver.notifyChange(PlaylistQuery.ALL, null);
  }

  @Nullable
  public final Cursor query(@Nullable long[] ids) {
    return resolver.query(PlaylistQuery.ALL, null,
        PlaylistQuery.selectionFor(ids), null, null);
  }

  public final void delete(long[] ids) {
    deleteItems(PlaylistQuery.selectionFor(ids));
    Analytics.sendPlaylistEvent(Analytics.PLAYLIST_ACTION_DELETE, ids.length);
  }

  public final void deleteAll() {
    deleteItems(null);
    Analytics.sendPlaylistEvent(Analytics.PLAYLIST_ACTION_DELETE, 0);
  }

  private void deleteItems(@Nullable String selection) {
    resolver.delete(PlaylistQuery.ALL, selection, null);
    notifyChanges();
  }

  public final void move(long id, int delta) {
    Provider.move(resolver, id, delta);
    notifyChanges();
    Analytics.sendPlaylistEvent(Analytics.PLAYLIST_ACTION_MOVE, 1);
  }

  public final void sort(SortBy by, SortOrder order) {
    Provider.sort(resolver, by.name(), order.name());
    notifyChanges();
    Analytics.sendPlaylistEvent(Analytics.PLAYLIST_ACTION_SORT,
        100 * by.ordinal() + order.ordinal());
  }

  //TODO: remove
  public static Loader<Cursor> createLoader(Context ctx) {
    return new CursorLoader(ctx, PlaylistQuery.ALL, null, null, null, null);
  }

  //TODO: remove
  public static Loader<Cursor> createStatisticsLoader(Context ctx, @Nullable long[] ids) {
    return new CursorLoader(ctx, PlaylistQuery.STATISTICS, null,
        PlaylistQuery.selectionFor(ids), null, null);
  }
}
