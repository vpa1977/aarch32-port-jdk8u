/*
 * Copyright (c) 2003, 2016, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 */

/*
 * @test
 * @bug 4868820
 * @library /lib/testlibrary
 * @build jdk.testlibrary.NetworkConfiguration
 * @summary IPv6 support for Windows XP and 2003 server
 * @run main TcpTest -d
 */

import java.net.*;
import java.io.*;
import jdk.testlibrary.NetworkConfiguration;

public class TcpTest extends Tests {
    static ServerSocket server, server1, server2;
    static Socket c1, c2, c3, s1, s2, s3;
    static InetAddress s1peer, s2peer;

    static InetAddress ia4any;
    static InetAddress ia6any;
    static Inet6Address ia6addr;
    static Inet4Address ia4addr;

    static {
        ia6addr = getFirstLocalIPv6Address ();
        ia4addr = getFirstLocalIPv4Address ();
        try {
            ia4any = InetAddress.getByName ("0.0.0.0");
            ia6any = InetAddress.getByName ("::0");
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    static void runNetstat() throws Exception {
        System.out.println("netstat -at");
        run ("netstat", "-atn");
    }

    static void runCatHosts() throws Exception  {
        System.out.println("cat /etc/hosts");
        run ("cat", "/etc/hosts");
    }

    static void runIpa()  throws Exception {
        System.out.println("ip a");
        run ("ip", "a");
    }


    static void run(String... args) throws Exception {
            Process process = new ProcessBuilder(args)
                .redirectErrorStream(true).start();
            int exitCode = process.waitFor();
            InputStream inputStream = process.getInputStream();
            BufferedReader reader = new BufferedReader(new InputStreamReader(inputStream));
            String line;
            while ((line = reader.readLine()) != null) {
                dprintln(line);
            }
            dprintln("exited with code: " + exitCode);
    }

    public static void main (String[] args) throws Exception {
        NetworkConfiguration.probe().printSystemConfiguration(System.out);
        checkDebug(args);
        if (ia6addr == null) {
            System.out.println ("No IPV6 addresses: exiting test");
            return;
        }
        runCatHosts();
        runIpa();
        dprintln ("Local Addresses");
        dprintln (ia4addr.toString());
        dprintln (ia6addr.toString());
       // test1();
        test3();
    }

    /* basic TCP connectivity test using IPv6 only and IPv4/IPv6 together */

    static void test1Ipv6(int port)  throws Exception {
        // try Ipv6 only
        dprintln("test1Ipv6(int port)");
        try {
            c1 = new Socket ("::1", port);
            s1 = server.accept ();
            dprintln("s1 = server.accept ();");
            runNetstat();
            simpleDataExchange (c1, s1);
        }
        finally {
            s1.close ();
            c1.close();
            dprintln("after close");
            runNetstat();
        }
    }

    static void test1Both(int port)  throws Exception {
        dprintln("test1Both(int port)");

        try {
            // try with both IPv4 and Ipv6
            c1 = new Socket ("127.0.0.1", port);//new Socket ("127.0.0.1", port);
            s1 = server.accept();
            dprintln("s1 = server.accept();");
            runNetstat();

            c2 = new Socket ("::1", port);//new Socket ("::1", port);
            s2 = server.accept();
            dprintln("s2 = server.accept();");
            runNetstat();

            s1peer = s1.getInetAddress();
            s2peer = s2.getInetAddress();

            if (s1peer instanceof Inet6Address) {
                t_assert ((s2peer instanceof Inet4Address));
                simpleDataExchange (c2, s1);
                simpleDataExchange (c1, s2);
            } else {
                t_assert ((s2peer instanceof Inet6Address));
                simpleDataExchange (c1, s1);
                simpleDataExchange (c2, s2);
            }
        }
        finally {
            c1.close();
            c2.close();
            s1.close();
            s2.close();
        }
    }

    static void test1 () throws Exception {
        server = new ServerSocket (0);
        int port = server.getLocalPort();
        dprintln("test1 - server local port " + port);
        test1Ipv6(port);
        test1Both(port);

        server.close ();
        dprintln("test 1 exit");
        runNetstat();
        System.out.println ("Test1: OK");
    }


    static void testTimeout() throws Exception {
        long t1 = System.currentTimeMillis();
        Socket timeout = null;
        try {
            timeout = server.accept ();
            throw new RuntimeException ("accept should not have returned");
        } catch (SocketTimeoutException e) {}
        finally {
            if (timeout != null)
                timeout.close();
        }
        t1 = System.currentTimeMillis() - t1;
        checkTime (t1, 5000);
        dprintln("accept should not have returned");
        runNetstat();
    }

    static void testIPv4Exchange(int port) throws Exception {
        try {
            c1 = new Socket ();
            c1.connect (new InetSocketAddress (ia4addr, port), 1000);
            s1 = server.accept ();
            dprintln("c1 : send: " + c1.getSendBufferSize() + " rec:" + c1.getReceiveBufferSize());
            dprintln("s1 : send: " + s1.getSendBufferSize() + " rec:" + s1.getReceiveBufferSize());
            dprintln("s1 = server.accept ();");
            runNetstat();
            dprintln("!simpleDataExchange (c1,s1);");
            simpleDataExchange (c1,s1);
        }
        finally {
            c1.close();
            s1.close();
            dprintln("c1.close();s2.close();");

            runNetstat();
        }
    }

    static void testIPv6Exchange(int port) throws Exception {
        try {
            c2 = new Socket ();
            c2.connect (new InetSocketAddress (ia6addr, port), 1000);
            s2 = server.accept ();

            dprintln("s2 = server.accept ();");
            runNetstat();
            try {
                simpleDataExchange (c2,s2);
            }
            catch (Exception e) {
                runNetstat();
                throw e;
            }
        }
        finally {
            c2.close();
            s2.close();
        }
    }


    static void test3 () throws Exception {
        dprintln("test1");
        server = new ServerSocket (0);
        int port = server.getLocalPort();
        dprintln("test1 - ipv6 conn");
        test1Ipv6(port);
        dprintln("test1 - ipv4 double conn");
        test1Both(port);
        server.close();
        server = new ServerSocket (0);
        server.setSoTimeout (5000);
         port = server.getLocalPort();
        dprintln("test3 - server local port " + port);

        dprintln("test3 - ipv4 conn");
        testIPv4Exchange(port);
        dprintln("test3 - ipv 6 conn");
        testIPv6Exchange(port);
        server.close();
        System.out.println ("Test3: OK");
    }

}

