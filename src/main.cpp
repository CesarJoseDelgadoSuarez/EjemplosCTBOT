#include <Arduino.h>
#include <CTBot.h>
#include <ArduinoJson.h>

String ssid = "tu-WiFi";
String pass = "Contraseña-Del-WiFi";
String token = "token-Bot-Telegram";

CTBot botTelegram;

CTBotInlineKeyboard tecladoUsuario;
CTBotInlineKeyboard tecladoGrupo;

long long idsPermitidos[] = {};    // Los usuarios suelen tener ids positivos
long long gruposPermitidos[] = {}; // Los grupos suelen tener ids negativos

#define boton1 "Botón 1"
#define boton2 "Botón 2"
#define boton1G "Botón 1 G"
#define boton2G "Botón 2 G"

void setup()
{
  Serial.begin(115200);
  // put your setup code here, to run once:
  if (botTelegram.wifiConnect(ssid, pass))
  {
    Serial.print("Conectado al WiFi: ");
    Serial.println(ssid);
  }
  else
  {
    Serial.println("No se pudo conectar al WiFi");
  }

  botTelegram.setTelegramToken(token);

  tecladoUsuario.addButton(boton1, boton1, CTBotKeyboardButtonQuery);
  tecladoUsuario.addButton(boton2, boton2, CTBotKeyboardButtonQuery);

  tecladoGrupo.addButton(boton1G, boton1G, CTBotKeyboardButtonQuery);
  tecladoGrupo.addButton(boton2G, boton2G, CTBotKeyboardButtonQuery);
}

/**
 * @return true si el id del usuario que ha mandado un mensaje esta permitido, false en caso contrario.
 *
 */
bool accesoUsuarios(long long id)
{
  for (int i = 0; i < sizeof(idsPermitidos) / sizeof(idsPermitidos[0]); i++)
  {
    if (idsPermitidos[i] == id)
    {
      return true;
    }
  }
  return false;
}

long long chatId = 0; // Solo usuarios
long long userId = 0; // Solo usuarios
// Delimita quien tiene acceso al bot segun el id del usuario que envió el mensaje
void restringirUsuarios(TBMessage mensaje)
{
  bool aceptado = accesoUsuarios(mensaje.sender.id);
  if (aceptado)
  {
    if (mensaje.messageType == CTBotMessageText)
    {
      Serial.println("Mensaje de tipo texto, obtenemos el user id");
      userId = mensaje.sender.id;
      if (mensaje.group.id < 0)
      {
        botTelegram.sendMessage(mensaje.group.id, "Estas hablando por un grupo. Habla por privado.");
      }
      else
      {
        if (mensaje.text.equalsIgnoreCase("t"))
        {
          botTelegram.sendMessage(chatId, "***Teclado***", tecladoUsuario);
        }
        else
        {
          botTelegram.sendMessage(chatId, "Envie T para obtener el teclado");
        }
      }
    }
    else if (mensaje.messageType == CTBotMessageQuery)
    {
      if (mensaje.callbackQueryData.equalsIgnoreCase(boton1))
      {
        Serial.println("Se ha pulsado el boton 1, realizando accion...");
        botTelegram.endQuery(mensaje.callbackQueryID, "Boton 1 pulsado", true); // pop-up con false, alerta con true
        userId = 0;
      }
      else if (mensaje.callbackQueryData.equalsIgnoreCase(boton2))
      {
        Serial.println("Se ha pulsado el boton 2, realizando accion...");
        botTelegram.endQuery(mensaje.callbackQueryID, "Boton 2 pulsado", false); // pop-up con false, alerta con true
        userId = 0;
      }
    }
  }
  else
  {
    botTelegram.sendMessage(mensaje.sender.id, "No estas autorizado o Estas hablando por un Grupo");
  }
}

/**
 * @return true si el id del grupo por el que se ha mandado un mensaje esta permitido, false en caso contrario.
 *
 */
bool accesoGrupos(long long id)
{
  for (int i = 0; i < sizeof(gruposPermitidos) / sizeof(gruposPermitidos[0]); i++)
  {
    if (gruposPermitidos[i] == id)
    {
      return true;
    }
  }
  return false;
}

long long chatIdG = 0; // Solo grupos
long long userIdG = 0; // Solo grupos
/**
 * Delimita quien tiene acceso al bot segun del chat donde se envio el mensaje
 * Ejemplo -> un usuario puede no tener acceso al bot si le habla por privado,
 * pero queremos que tenga acceso mediante un grupo para asi monitorear su acciones,
 * o incluso a un mismo usuario se le manden teclados distintos segun por donde mande el mensaje.
 *
 * OJO IMPORTANTE!!!!
 * A la hora de obtener el id del grupo es importante saber que:
 *  - Si el usuario habla por privado el id del grupo es el mismo id del usuario es decir, mensaje.group.id == mensaje.sender.id
 *  - Si el usuario habla por un grupo el id del grupo es distinto del id del usuario, es decir, mensaje.group.id != mensaje.sender.id
 *  - El id del grupo no se obtiene correctamente mediante mensaje tipo Query, por lo que si el bot debe mandar un mensaje de confirmación
 *    es importante que el id del grupo se obtenga del mensaje tipo Text(no se ha probado con los otros tipos de mensajes).
 */
void resitringirGrupos(TBMessage mensaje)
{
  bool aceptado = accesoGrupos(mensaje.group.id);
  if (aceptado)
  {
    if (mensaje.messageType == CTBotMessageText)
    {
      Serial.println("Mensaje de tipo texto, obtenemos el user id");

      if (mensaje.text.equalsIgnoreCase("t"))
      {
        if (userIdG != 0)
        {
          String s = mensaje.sender.firstName;
          s += ", hay alguien con el permiso para usar el bot, espera a que termine.";
          botTelegram.sendMessage(chatIdG, s);
        }
        else
        {
          userId = mensaje.sender.id;
          chatId = mensaje.group.id;
          botTelegram.sendMessage(chatIdG, "***Teclado***", tecladoGrupo);
        }
      }
      else if (mensaje.text.equalsIgnoreCase("p"))
      {
        if (userIdG != 0)
        {
          String s = mensaje.sender.firstName;
          s += ", hay alguien con el permiso para usar el bot, espera a que termine.";
          botTelegram.sendMessage(chatIdG, s);
        }
        else
        {
          String s = mensaje.sender.firstName;
          s += ", tienes el turno para usar el bot.";
          botTelegram.sendMessage(mensaje.group.id, s);
          userId = mensaje.sender.id;
          chatId = mensaje.group.id;
        }
      }
      else
      {
        botTelegram.sendMessage(mensaje.group.id, "Envie T para obtener el teclado");
      }
    }
    else if (mensaje.messageType == CTBotMessageQuery)
    {
      if (userIdG == mensaje.sender.id)
      {
        if (mensaje.callbackQueryData.equalsIgnoreCase(boton1))
        {
          Serial.println("Se ha pulsado el boton 1, realizando accion...");
          botTelegram.endQuery(mensaje.callbackQueryID, "Boton 1 pulsado", true); // pop-up con false, alerta con true
          chatIdG = 0;
          userIdG = 0;
        }
        else if (mensaje.callbackQueryData.equalsIgnoreCase(boton2))
        {
          Serial.println("Se ha pulsado el boton 2, realizando accion...");
          botTelegram.endQuery(mensaje.callbackQueryID, "Boton 2 pulsado", false); // pop-up con false, alerta con true
          chatIdG = 0;
          userIdG = 0;
        }
      }
      else
      {
        botTelegram.sendMessage(mensaje.sender.id, "Hay alguien mas con el permiso de usar el bot, Para obtener el turno envia por el grupo P o T.");
      }
    }
  }
  else
  {
    botTelegram.sendMessage(mensaje.sender.id, "Este grupo no esta autorizado o Estas hablando por privado");
  }
}



long long chatIdA = 0; // Ambos
long long userIdA = 0; // Ambos
bool flagGrupo=false;
void gestionMensajesAmbos(long long id, TBMessage mensaje)
{
  if (mensaje.messageType==CTBotMessageText)
  {
    if (id > 0)
    {
      if (mensaje.text.equalsIgnoreCase("t"))
      {
        botTelegram.sendMessage(id,"***Teclado Usuario***", tecladoUsuario);
      }else{
        botTelegram.sendMessage(id,"Prueba con T.");
      }
      
    }else{
      if (mensaje.text.equalsIgnoreCase("t"))
      {
        if(userIdA!=0){
          botTelegram.sendMessage(id,"Hay alguien usando el bot, intenta pedir el teclado mas tarde.");
        }else{
          botTelegram.sendMessage(id,"***Teclado Grupo***", tecladoGrupo);
          userIdA = mensaje.sender.id;
          chatIdA = id;
          flagGrupo=true;
        }
      }else if(mensaje.text.equalsIgnoreCase("p")){
        if(userIdA!=0){
          botTelegram.sendMessage(id,"Hay alguien usando el bot, intenta pedir turno mas tarde.");
        }else{
          String s = mensaje.sender.firstName;
          s += ", tienes permiso para usar el bot.";
          botTelegram.sendMessage(id,s);
          userIdA = mensaje.sender.id;
          chatIdA = id;
          flagGrupo=true;
        }
      }else
      {
        botTelegram.sendMessage(id,"Prueba con T o P.");
      }
    }
  } else if(mensaje.messageType==CTBotMessageQuery) {
    if(chatIdA==0){
      if(mensaje.callbackQueryData.equals(boton1)){
        Serial.println("Accion boton 1 de usuario...");
      }else if(mensaje.callbackQueryData.equals(boton2)){
        Serial.println("Accion boton 2 de usaurio...");
      }
    }else{
      if(userIdA==mensaje.sender.id){
        if(mensaje.callbackQueryData.equals(boton1G)){
          Serial.println("Accion boton 1 de grupo...");
          userIdA=0;
          chatIdA=0;
          flagGrupo=false;
        }else if(mensaje.callbackQueryData.equals(boton2G)){
          Serial.println("Accion boton 2 de grupo...");
          userIdA=0;
          chatIdA=0;
          flagGrupo=false;
        }
      }
    }
  }
}

void restringirUserYGrupos(TBMessage mensaje)
{
  bool usuarioP = accesoUsuarios(mensaje.sender.id);
  bool grupoP = accesoGrupos(mensaje.group.id);
  // Usuario permitido y gurpo no permitido, se esta hablando por mensaje directo
  if (usuarioP && !grupoP)
  {
    gestionMensajesAmbos(mensaje.sender.id, mensaje);
  }
  else if (!usuarioP && grupoP || usuarioP && grupoP || flagGrupo)
  { // usuario no permitido y grupo permitido, hablando por grupo con un usuario sin autorizar(esta permitido este caso)
    if(mensaje.group.id<0){
      gestionMensajesAmbos(mensaje.group.id, mensaje);
    }else{
      gestionMensajesAmbos(chatIdA, mensaje);
    }
  }
  else
  { // en caso de que alguien/grupo sin autorizacion hable con el bot
    botTelegram.sendMessage(mensaje.group.id, "No estas autorizado");
    String s = mensaje.sender.firstName;
    s += ", intento acceder al bot.";
    if (mensaje.group.id > 0)
    {
      s += " Por privado.";
    }
    else
    {
      s += " Por un grupo.";
    }
    botTelegram.sendMessage(gruposPermitidos[0], s);
  }
}

void loop()
{
  TBMessage mensaje;
  if (botTelegram.getNewMessage(mensaje))
  {
    // restringirUsuarios(mensaje);
    // resitringirGrupos(mensaje);
    restringirUserYGrupos(mensaje);
  }
}