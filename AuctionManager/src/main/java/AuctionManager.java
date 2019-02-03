import com.google.gson.Gson;
import com.sun.net.httpserver.HttpContext;
import com.sun.net.httpserver.HttpExchange;
import com.sun.net.httpserver.HttpServer;
import org.apache.http.HttpResponse;
import org.apache.http.client.HttpClient;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.entity.ContentType;
import org.apache.http.entity.StringEntity;
import org.apache.http.impl.client.HttpClientBuilder;
import org.apache.http.util.EntityUtils;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.InetSocketAddress;
import java.net.URI;
import java.nio.channels.Selector;
import java.nio.channels.SocketChannel;
import java.nio.charset.Charset;
import java.sql.Timestamp;
import java.util.*;




public class AuctionManager {
    //TODO: CLEAN THESE UNUSED VARIABLES
    private Selector selector;
    private Map<SocketChannel,List> dataMapper;
    private InetSocketAddress listenAddress;
    private static HashMap<String, Settings> auctionSettings = new HashMap<>();
    private static HashMap<String, String> auctionPasswordsE = new HashMap<>();
    private static HashMap<String, String> auctionPasswordsD = new HashMap<>();
    private static Integer RepositoryPort = 8501;
    private static Integer ManagerPort = 8500;


    public static void main(String[] args) throws Exception {



// HTTP SERVER STUFF
        HttpServer server = HttpServer.create(new InetSocketAddress(8500), 0);
        HttpContext context = server.createContext("/security");
        context.setHandler(AuctionManager::handleRequest);
        server.start();
    }

    private static void handleRequest(HttpExchange exchange) throws IOException {
        Timestamp timestampReception = new Timestamp(System.currentTimeMillis());
        URI requestURI = exchange.getRequestURI();
        //printRequestInfo(exchange);
        System.out.println("This is the response at " + requestURI);
        if (exchange.getRequestMethod().equalsIgnoreCase("POST")) {
            System.out.println(" \n          POST ");
            String received;

            received = convert(exchange.getRequestBody(), Charset.forName("UTF-8"));
            System.out.println("received: "+received);
            Gson gson = new Gson();
            //System.out.println(gsonReceived);
            Message receivedMessage = gson.fromJson(received, Message.class);

            // RECEIVED BID FROM REPOSITORY
            if (receivedMessage.getAction().equals("addBid")){
                System.out.println(" Checking bid...");

                Bid receivedBid = gson.fromJson(receivedMessage.getData(), Bid.class);
                Settings auxSettings = auctionSettings.get(receivedBid.getAuction_number());

                if (auxSettings.isEncryptedBidder()){
                    //Encrypt bidder
                    receivedBid.setFrom(""/*todo: */);
                }
                if (auxSettings.isEncryptedBidValue()){
                    //Encrypt bid value
                    receivedBid.setBid_value("Encrypted");
                }
                if (auxSettings.isEncryptedAuthor()){
                    //Encrypt author
                }
                String response = "good";
                System.out.println("response:"+response);
                //USAR O receivedAuction.get(...) para usar os dados no repositório e no manager

                //envio a resposta ao pedido de Post, neste caso, metendo esta string construída com o que se obtem do novo objecto Auction
                exchange.sendResponseHeaders(200, response.length());
                OutputStream os = exchange.getResponseBody();
                os.write(response.getBytes());
                os.close();
                System.out.println("Bid added...");
            }
            if (receivedMessage.getAction().equals("terminateAuction")) {
                System.out.println("Terminating... ");
                String response = "";
                String uuid = receivedMessage.getData();
                Message message = new Message("Auction", "terminateAuction", uuid);
                String stringMessage = gson.toJson(message);
                try {
                    //sends the auction in to the manager to close it
                    response = send(stringMessage, RepositoryPort);
                }
                catch (Exception e123){ }

                exchange.sendResponseHeaders(200, response.length());
                OutputStream os = exchange.getResponseBody();
                os.write(response.getBytes());
                os.close();
                System.out.println("Request to terminate sent...");

            }

            if (receivedMessage.getAction().equals("addAuction"))
            {
                System.out.println("Adding auction...");
                Auction receivedAuction = gson.fromJson(receivedMessage.getData(), Auction.class);
                UUID uuid = UUID.randomUUID();
                receivedAuction.setId(uuid.toString());
                System.out.println(receivedAuction.getId());
                Settings receivedSettings = gson.fromJson(receivedAuction.getSettings(), Settings.class);

                //Guarda os settings
                auctionSettings.put(receivedAuction.getId(), receivedSettings);

                //enviar receivedAuction para o Repositório
                String stringAuction = gson.toJson(receivedAuction);

                // Enviar para o repositório
                Message message = new Message("Auction", "addAuction", stringAuction);
                String stringMessage = gson.toJson(message);
                System.out.println("Enviando para o repositório" + stringMessage);
                try {
                    send(stringMessage,RepositoryPort);
                }catch (Exception en) {
                }

                // SEND RESPONSE TO CLIENT
                String response = ("Auction id: " + receivedAuction.getId() + "\n" + "Auction creation timestamp: " + receivedAuction.getTimestamp() + "\n" + "Timestamp reception: " + timestampReception + "\n");
                System.out.println(response);
                System.out.println(receivedAuction.getProduct());
                //USAR O receivedAuction.get(...) para usar os dados no repositório e no manager

                //envio a resposta ao pedido de Post, neste caso, metendo esta string construída com o que se obtem do novo objecto Auction
                exchange.sendResponseHeaders(200, response.length());
                OutputStream os = exchange.getResponseBody();
                os.write(response.getBytes());
                os.close();
            }

        }
        //TO DO: tirar no final
        if (exchange.getRequestMethod().equalsIgnoreCase("GET")) {
            System.out.println(" GET ");

            String received = "";
            String response = "";
            received = convert(exchange.getRequestBody(), Charset.forName("UTF-8"));
            Gson gsonReceived = new Gson();
            Message message = gsonReceived.fromJson(received, Message.class);
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
    private static String send(String message2send, Integer port) throws Exception{


        // HTTP code
        StringEntity entity = new StringEntity(message2send,
                ContentType.APPLICATION_JSON);

        HttpClient httpClient = HttpClientBuilder.create().build();
        HttpPost request = new HttpPost("http://localhost:"+port+"/security"); //COLOCAR ISTO MAIS PARA CIMA, NUMA COISA TIPO #DEFINE
        request.setEntity(entity);


        HttpResponse response = httpClient.execute(request);
        String responseBody = EntityUtils.toString(response.getEntity());
        return responseBody;


    }


}

