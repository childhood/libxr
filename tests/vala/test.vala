using GLib;
using T;
using XR;

namespace T
{
  static void test1() throws GLib.Error
  {
    var c = new T.Test1();
    c.open("https://localhost:4444");

    var all = c.getAll();
    var alla = c.getAllArrays();
    
    foreach (weak string s in alla.a_string)
      stdout.printf("S: %s\n", s);

    c.close();
  }

  static void test2() throws GLib.Error
  {
  }

  static void test3() throws GLib.Error
  {
    var call = new Call("test");
    call.add_param(new XR.Value.int(100));
    call.add_param(new XR.Value.string("test"));
    call.add_param(new XR.Value.string("par2"));
    call.add_param(new XR.Value.blob(new XR.Blob("bloooooob", -1)));
    var retval = call.get_retval();
    if (retval != null)
    {
      int v;
      if (!retval.to_int(ref v))
        return;
    }
    return;

    if (retval.get_type() == ValueType.ARRAY)
      return;
  }

  static int main()
  {
    XR.init();

    try 
    {
      test1();
      test2();
      test3();
    } 
    catch(GLib.Error e)
    {
      stderr.printf("ERROR: %d: %s\n", e.code, e.message);
    }
    
    XR.fini();
    return 0;
  }
}
