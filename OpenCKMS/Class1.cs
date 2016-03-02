using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using cryptlib;

namespace OpenCKMS
{
    public class Class1
    {
        public void T()
        {
            crypt.Init();
            var context = crypt.CreateContext(crypt.UNUSED, crypt.ALGO_RSA);
            crypt.SetAttributeString(context, crypt.CTXINFO_LABEL, "keys2");
            crypt.GenerateKey(context);
            var keyStore = crypt.KeysetOpen(crypt.UNUSED, crypt.KEYSET_FILE, @"C:\Temp\PrivateKeys.db", crypt.KEYOPT_NONE);
            crypt.AddPrivateKey(keyStore, context, "P@ssw0rd");
            crypt.KeysetClose(keyStore);
            crypt.DestroyContext(context);
            crypt.End();

        }
    }
}
