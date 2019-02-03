import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.sun.net.httpserver.*;
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
import java.nio.charset.Charset;
import java.sql.Timestamp;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Scanner;

public class AuctionRepository {

    //Create ArrayList, named blockchain with objects of type Block

    private static HashMap<String, ArrayList<Block>> active_blockchains = new HashMap<>();
    private static HashMap<String, ArrayList<Block>> terminatedBlockchain = new HashMap<>();

    private static Integer RepositoryPort = 8501;
    private static Integer ManagerPort = 8500;

    //Ammount of blocks to add to the blockchain, for testing
    private static final int BLOCKSTOADD = 4;

    public static void main(String[] args) throws IOException {

        // HTTP SERVER STUFF
        HttpServer server = HttpServer.create(new InetSocketAddress(8501), 0);
        HttpContext context = server.createContext("/security");
        context.setHandler(AuctionRepository::handleRequest);
        server.start();

    }

    private static void handleRequest(HttpExchange exchange) throws IOException {
        Timestamp timestampReception = new Timestamp(System.currentTimeMillis());

        URI requestURI = exchange.getRequestURI();
        //printRequestInfo(exchange);
        //String response = "This is the response at " + requestURI;
        if (exchange.getRequestMethod().equalsIgnoreCase("POST")) {
            System.out.println(" \n          POST ");
            String received;
            String response = "";
            received = convert(exchange.getRequestBody(), Charset.forName("UTF-8"));
            System.out.println(received);
            Gson gsonreceived = new Gson();
            System.out.println("Reached the after POST");
            Message message = gsonreceived.fromJson(received, Message.class);
            switch (message.getAction()) {

                case "addAuction":
                    ArrayList<Block> blockchain = new ArrayList<Block>();
                    Auction receivedAuction = gsonreceived.fromJson(message.getData(), Auction.class);
                    System.out.println(receivedAuction.getId());

                    //SEND THIS TO THE BLOCk

                    //Adding the genesis block to the blockchain
                    blockchain.add(new Block(receivedAuction, "0")); // First block's previous hash is 0 because it's the genesis block.
                    //blockchain.add(new Block(receivedAuction,(blockchain.get(blockchain.size()-1).hash)));
                    //Stores the blockchain in JSON format, with PrettyPrinting on(facilitating human reading).
                    String blockchainJSON = new GsonBuilder().setPrettyPrinting().create().toJson(blockchain);

                    //Prints the JSON
                    // System.out.println(blockchainJSON);

                    //System.out.println("Is the blockchain valid: " + isBlockChainValid(blockchain));
                    active_blockchains.put(receivedAuction.getId(), blockchain);
                    System.out.println("actove: "+active_blockchains + "\n" + "  receivedAuction: "+receivedAuction + "\n blockchain: "+blockchain);

                    response = ("Auction id: " + receivedAuction.getId() + "\n" + "Auction creation timestamp: " + receivedAuction.getTimestamp() + "\n" + "Timestamp reception: " + timestampReception + "\n");
                    System.out.println(response);
                    break;
                case "List":
                    System.out.println("listing...");
                    response = gsonreceived.toJson(active_blockchains);
                    break;
                case "terminateAuction":
                    System.out.println("Terminating...");
                    terminatedBlockchain.put(message.getData(),active_blockchains.get(message.getData()));
                    active_blockchains.remove(message.getData());
                    response = "removed";
                    break;
                case "listInBids":
                    System.out.println("listing...");
                    response = gsonreceived.toJson(active_blockchains);
                    break;
                case "addBid":
                    System.out.println("adding bid...");
                    String responseBidToManager="tbd";
                    Gson gson = new Gson();
                    Bid receivedBid = gsonreceived.fromJson(message.getData(), Bid.class);
                    System.out.println("Sending "+receivedBid.getAuction_number()+" to the  manager for verification");
                    String stringBid = gson.toJson(receivedBid);
                    message = new Message("Bid", "addBid", stringBid);
                    String stringMessage = gson.toJson(message);
                    try {
                        //send it to the manager to check
                        responseBidToManager = postMessage(stringMessage, ManagerPort);
                        System.out.println("response:" + responseBidToManager);
                    }catch (Exception en) {
                        System.out.println(en);
                    }
                    System.out.println("responseBidToManager: "+responseBidToManager);
                    if(responseBidToManager.equals("good"))
                    {
                        System.out.println("active: "+active_blockchains);
                        ArrayList<Block> blockchain_active = active_blockchains.get(receivedBid.getAuction_number());
                        System.out.println("number:" + receivedBid.getAuction_number());
                        System.out.println("check");
                        for(int i=0;i<blockchain_active.size();i++)
                        {
                            System.out.println(i + " " +blockchain_active.get(i).hash);
                        }


                        blockchain_active.add(blockchain_active.size(),new Block(receivedBid,blockchain_active.get(blockchain_active.size()-1).hash));
                        if(isBlockChainValid(blockchain_active))
                        {
                            System.out.println("Sucess, bid entered!Blockchain is valid");
                            response = "Bid entered blockchain...";
                            System.out.println(response);
                        }
                        else
                        {
                            System.out.println("Error, blockchain invalid");
                            response = "Bid did not enter blockchain...";
                            System.out.println(response);
                        }

                    }
                    else
                    {
                        response = "good = false Bid did not enter blockchain...";
                        System.out.println(response);

                    }
                    break;
                        //Stores the blockchain in JSON format, with PrettyPrinting on(facilitating human reading).
            }//USAR O receivedAuction.get(...) para usar os dados no repositório e no manager
            System.out.println("response:"+response);
            //envio a resposta ao pedido de Post, neste caso, metendo esta string construída com o que se obtem do novo objecto Auction
            exchange.sendResponseHeaders(200, response.length());
            OutputStream os = exchange.getResponseBody();
            os.write(response.getBytes());
            os.close();


        }

        if(exchange.getRequestMethod().equalsIgnoreCase("GET"))
        {
            System.out.println("GET");

            URI uri = exchange.getRequestURI();

            System.out.println("uri: "+uri.getQuery());

            String response;
            switch (uri.getQuery())
            {
            case "=open":
                //send both repositories

                if(active_blockchains.isEmpty())
                {
                    response="empty";
                }
                else {
                    response = new GsonBuilder().setPrettyPrinting().create().toJson(active_blockchains);
                }
                break;

            case "=closed":
            {
                //send both repositories
                if(terminatedBlockchain.isEmpty())
                {
                    response="empty";
                }
                else {
                    response = new GsonBuilder().setPrettyPrinting().create().toJson(terminatedBlockchain);
                }
                break;

            }
                default:
                response = "ERROR, wrong query ("+requestURI.getQuery()+" \n " + "Time: " + timestampReception;
            }

            System.out.println("response:" + response);
            exchange.sendResponseHeaders(200, response.getBytes().length);
            OutputStream os = exchange.getResponseBody();
            os.write(response.getBytes());
            os.close();
        }


    }

    private static void printRequestInfo(HttpExchange exchange) {
        System.out.println("-- headers --");
        Headers requestHeaders = exchange.getRequestHeaders();
        requestHeaders.entrySet().forEach(System.out::println);

        System.out.println("-- principle --");
        HttpPrincipal principal = exchange.getPrincipal();
        System.out.println(principal);

        System.out.println("-- HTTP method --");
        String requestMethod = exchange.getRequestMethod();
        System.out.println(requestMethod);

        System.out.println("-- query --");
        URI requestURI = exchange.getRequestURI();
        String query = requestURI.getQuery();
        System.out.println(query);


    }

    //Method to test blockchain's validity
    public static Boolean isBlockChainValid(ArrayList<Block> blockchain) {
        //Create the Blocks objects to use next
        Block previous_block, current_block;

        //Check each element of the blockchain for it's hash (starts at 1, because the first element has "0" as previous hash - the genesis block
        for (int i = 1; i < blockchain.size(); i++) {

            current_block = blockchain.get(i);
            previous_block = blockchain.get(i - 1);

            //compare the hash on the block to the calculated hash:
            if (!current_block.hash.equals(current_block.calculateHash())) {
                System.out.println("Current hash is different for block in position number " + i + " in the blockchain.");
                return false;
            }
            if (!previous_block.hash.equals(current_block.previous_hash)) {
                System.out.println("Previous hash is different for block in position number " + i + " in the blockchain.");
                return false;
            }

        }
        return true;
    }

    public static String convert(InputStream inputStream, Charset charset) throws IOException {

        try (Scanner scanner = new Scanner(inputStream, charset.name())) {
            return scanner.useDelimiter("\\A").next();
        }
    }
    private static String postMessage(String mensage, Integer port) throws Exception {


        // HTTP code
        StringEntity entity = new StringEntity(mensage, ContentType.APPLICATION_JSON);

        HttpClient httpClient = HttpClientBuilder.create().build();
        HttpPost request = new HttpPost("http://localhost:" + port + "/security");
        request.setEntity(entity);

        Gson gson = new Gson();

        HttpResponse response = httpClient.execute(request);
        //Message message = gson.fromJson(mensage, Message.class);
        //System.out.println(message.getAction());
        /*switch (message.getAction()){
            case "listInBids":
                System.out.println(";:)");

                Type type = new TypeToken<HashMap<String, ArrayList<Block>>>(){}.getType();

                String responseBody = convert(response.getEntity().getContent(),Charset.forName("UTF-8"));
                System.out.println("response:"+responseBody);
                HashMap<String, ArrayList<Block>> active_blockchains = gson.fromJson(responseBody, type);

                System.out.println(active_blockchains.size());
                for (HashMap.Entry<String, ArrayList<Block>> entry : active_blockchains.entrySet()) {

                    String key = entry.getKey();
                    ArrayList<Block> value = entry.getValue();
                    System.out.println(value.get(0).auction.getProduct());
                    comboBoxAuction.addItem(value.get(0).auction.getProduct());
                    // ...
                }
        }*/
        String responseBody = EntityUtils.toString(response.getEntity());
        return responseBody;
    }

}

