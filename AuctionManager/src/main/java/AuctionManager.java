import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.SocketAddress;
import java.net.URI;
import java.nio.ByteBuffer;
import java.nio.channels.SelectionKey;
import java.nio.channels.Selector;
import java.nio.channels.ServerSocketChannel;
import java.nio.channels.SocketChannel;
import java.nio.charset.Charset;
import java.sql.Timestamp;
import java.util.*;

import com.google.gson.Gson;
import com.sun.net.httpserver.*;


public class AuctionManager {
    private Selector selector;
    private Map<SocketChannel,List> dataMapper;
    private InetSocketAddress listenAddress;

    public static void main(String[] args) throws Exception {


// HTTP SERVER STUFF
        HttpServer server = HttpServer.create(new InetSocketAddress(8500), 0);
        HttpContext context = server.createContext("/example");
        context.setHandler(AuctionManager::handleRequest);
        server.start();
    }

    private static void handleRequest(HttpExchange exchange) throws IOException {
        Timestamp timestampReception = new Timestamp(System.currentTimeMillis());
        URI requestURI = exchange.getRequestURI();
        //printRequestInfo(exchange);
        //String response = "This is the response at " + requestURI;
        if (exchange.getRequestMethod().equalsIgnoreCase("POST")) {
            System.out.println(" \n          POST ");
            String received = "";

            received = convert(exchange.getRequestBody(), Charset.forName("UTF-8"));

            Gson gsonReceived = new Gson();
            System.out.println(gsonReceived);
            Message receivedMessage = gsonReceived.fromJson(received, Message.class);
            Gson gsonAuction = new Gson();
            Auction receivedAuction = gsonAuction.fromJson(receivedMessage.getData(), Auction.class);
            System.out.println(receivedAuction.getProduct());
            String response = ("Auction id: " + receivedAuction.getId() + "\n" + "Auction creation timestamp: " + receivedAuction.getTimestamp() + "\n" + "Timestamp reception: " + timestampReception + "\n");
            System.out.println(response);
            //USAR O receivedAuction.get(...) para usar os dados no repositório e no manager

            //envio a resposta ao pedido de Post, neste caso, metendo esta string construída com o que se obtem do novo objecto Auction
            exchange.sendResponseHeaders(200, response.length());
            OutputStream os = exchange.getResponseBody();
            os.write(response.getBytes());
            os.close();


        }

        if (exchange.getRequestMethod().equalsIgnoreCase("GET")) {
            System.out.println(" GET ");

            String received = "";
            String response = "";

            //Figure out what they want with this GET method
            //TO DO: DO SOMETHING WITH THE GET REQUEST

            response = "We received your GET request. Unfortunately, Madaíl hasn't done this part yet and I'm tired \n " + "Time: " + timestampReception;
            System.out.println(response);
            exchange.sendResponseHeaders(200, response.getBytes().length);
            OutputStream os = exchange.getResponseBody();
            os.write(response.getBytes());
            os.close();
        }


    }
    public static String convert(InputStream inputStream, Charset charset) throws IOException {

        try (Scanner scanner = new Scanner(inputStream, charset.name())) {
            return scanner.useDelimiter("\\A").next();
        }
    }
        /*
        Runnable server = new Runnable() {
            @Override
            public void run() {
                try {
                    new AuctionManager("localhost", 8090).startServer();
                } catch (IOException e) {
                    e.printStackTrace();
                }

            }
        };

        new Thread(server).start();

    }

    public AuctionManager(String address, int port) throws IOException {
        listenAddress = new InetSocketAddress(address, port);
        dataMapper = new HashMap<SocketChannel,List>();
    }

    // create server channel
    private void startServer() throws IOException {
        this.selector = Selector.open();
        ServerSocketChannel serverChannel = ServerSocketChannel.open();
        serverChannel.configureBlocking(false);

        // retrieve server socket and bind to port
        serverChannel.socket().bind(listenAddress);
        serverChannel.register(this.selector, SelectionKey.OP_ACCEPT);

        System.out.println("Server started...");

        while (true) {
            // wait for events
            this.selector.select();

            //work on selected keys
            Iterator keys = this.selector.selectedKeys().iterator();
            while (keys.hasNext()) {
                SelectionKey key = (SelectionKey) keys.next();

                // this is necessary to prevent the same key from coming up
                // again the next time around.
                keys.remove();

                if (!key.isValid()) {
                    continue;
                }

                if (key.isAcceptable()) {
                    this.accept(key);
                }
                else if (key.isReadable()) {
                    this.read(key);
                }
            }
        }
    }

    //accept a connection made to this channel's socket
    private void accept(SelectionKey key) throws IOException {
        ServerSocketChannel serverChannel = (ServerSocketChannel) key.channel();
        SocketChannel channel = serverChannel.accept();
        channel.configureBlocking(false);
        Socket socket = channel.socket();
        SocketAddress remoteAddr = socket.getRemoteSocketAddress();
        System.out.println("Connected to: " + remoteAddr);

        // register channel with selector for further IO
        dataMapper.put(channel, new ArrayList());
        channel.register(this.selector, SelectionKey.OP_READ);
    }

    //read from the socket channel
    private void read(SelectionKey key) throws IOException {
        SocketChannel channel = (SocketChannel) key.channel();
        ByteBuffer buffer = ByteBuffer.allocate(1024);
        int numRead = -1;
        numRead = channel.read(buffer);

        if (numRead == -1) {
            this.dataMapper.remove(channel);
            Socket socket = channel.socket();
            SocketAddress remoteAddr = socket.getRemoteSocketAddress();
            System.out.println("Connection closed by client: " + remoteAddr);
            channel.close();
            key.cancel();
            return;
        }

        byte[] data = new byte[numRead];
        System.arraycopy(buffer.array(), 0, data, 0, numRead);
        System.out.println("Got: " + new String(data));
    }
}/*
import java.io.IOException;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.lang.ClassNotFoundException;
import java.net.InetAddress;
import java.net.Socket;
import java.net.SocketException;

//TO DO: WAIT UNTIL REPOSITORY IS CONNECTED TO CONNECT
public class AuctionManager {
    public static void main(String[] args) {
        try {
            // Create a connection to the server socket on the server application
            InetAddress host = InetAddress.getLocalHost();
            Socket socket = new Socket(host.getHostName(), 7777);

            // Send a message to the client application
            ObjectOutputStream oos = new ObjectOutputStream(socket.getOutputStream());
            oos.writeObject("Hello There...");

            // Read and display the response message sent by server application
            ObjectInputStream ois = new ObjectInputStream(socket.getInputStream());
            String message = (String) ois.readObject();
            System.out.println("Message: " + message);

            ois.close();
            oos.close();
        } catch (IOException | ClassNotFoundException e) {

        }*/

}

