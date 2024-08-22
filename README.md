# SinucaBalistica



![image](https://github.com/user-attachments/assets/c23f7924-431f-457d-8458-bf1335ff10e8)

![image](https://github.com/user-attachments/assets/46a1abaf-d778-4391-b58e-59958efda527)



Trabalho final para a cadeira de Fundamentos da Computação Gráfica
UFRGS, 2024


Desenvolvimento:
    As primeiras versões do projeto foram feitas em cima do código do laboratório 4, já com a implementação do sistema de iluminação, e carregamento de objetos utilizando o Tiny Object Loader. As primeiras horas de desenvolvimento foram dedicadas a crição de uma estrutura de dados para conter os objetos móveis dentro do jogo, permitindo sua interação com a física e renderização mais flexível. Tendo um sistema para representar as bolinhas de sinuca, foi implementado um sistema de física para gerenciar o movimento segundo o tempo, as colisões com os planos da mesa e a colisão entra as bolinhas. A colisão esfera-esfera, além de detectar o ponto de colisão, distribui os momentos dos objetos de acordo com a lógica de colisões perfeitamente elásticas, permitindo a física realista das esferas. Esse sistema era inicialmente preso em um unico plano y, mas sua robustez se revelou quando colisões utilizando o eixo y puderam ser habilitadas facilmente. Após isso, equações de colisão raio-esfera foram utilizadas para fazer um raycast a partir da posição da câmera, para aplicar vetores de impulso às bolinhas, quando um tiro se encontra com algum objeto. A colisão final para o jogo de sinuca ser possível, a colisão com buracos, foi uma das últimas a ser implementadas, seguida da colisão do jogador com a mesa, de menor importância para a jogabilidade.
    O sistema de câmera foi baseado no código do laboratório 2, sendo dividido em valores independentes para a camera livre e a câmera look-at. Valores de ângulo, providos do movimento do cursor, também são calculados separadamente. Curvas de Bezier foram implementadas para permitir a transição da free camera para uma look-at de forma mais suave. A camera look-at perpetuamente olha para a bola branca, que não tem a colisão com os buracos.
    Tendo então dificuldade em aplicar texturas a novos objetos, o projeto foi rebaseado no código do laboratório 5. Assim, foram inseridos objetos da mesa de sinuca, e da sala, ambos vindos de modelos gratuítos disponíveis na plataforma Sketchfab. Tendo eles em seu lugar, foi inseria a arma, e seu movimento reativo á posição dos jogador. As animações de zoom-in da arma foram feitas com curvas de bezier e interpolação linear de pontos e valores como o FieldOfView, assim como na velocidade do cursor e do personagem. Enquanto isso era feito, também houve um progresso no sistema para corretamente passar as texturas das bolinhas para o shader, e aplicar os diferentes modelos de iluminação nos objetos. Tendo todas as texturas e modelos de iluminação, bastou somente fazer algumas melhorias na qualidade do jogo, estabilizar algumas peculiaridades da física, limpar alguns bugs e, como as últimas features, implementar som e uma segunda opção de arma. Como qualidade de vida, foi dado um multiplicador para o tiro de abertura do jogo, e habilitada a opção para resetar a mesa de sinuca.

Contribuições:
  Miguel:
    Sistema de física
    Colisões em geral
    Movimento da camera
    Animações & Bezier
    Armas e tiro
    Seleção dos modelos 3D
    Sistema de som
    
  Nathan:
    Aplicação das texturas as bolinhas
    Sistema de texturas em geral
    Fragment shader & Vertex shader
    Iluminação Lambert e Blin-phong
    Shading de Phong e Gouraud

Não foi utilizada a ferramenta ChatGPT para a criação deste projeto.

Compilação do trabalho.
    O projeto pode ser compilado utilizando o Visual Studio Code. É recomendado o uso da extensão Cmake, e a utilização de algum compilador que não venha junto com a Raylib (Por referência, foi utilizado uo MinGW provido da instalação de Perl). O projeto deve ser clonado por git, ou desempacotado a partir do zip. Os arquivos CMake devem tornar a compilação simples pelo Visual Studio Code. 

Tutorial para o uso do programa:
  Teclas e ações:
    Movimento do mouse -> movimento da câmera
    Botão esquerdo do mouse -> atirar
    Botão direito do mouse -> mira com zoom
    Roda do mouse -> zoom quando no modo Look-At
    F -> troca entre free camera e look-at
    Y -> reseta a mesa de sinuca
    W,A,S,D -> movimentação horizontal
    Shift, control esquerdos -> movimentação vertical
    1 -> altera para arma Rifle
    0 -> altera para arma Pistola

    P -> Projeção perspectiva 
    O -> Projeção ortogonal (não recomendado)
    R -> Recarrega shaders

  Objetivos:
      O jogo é de estilo sandbox, então não tem um objetivo fixo. Como sugestão, uma variação da sinuca para um jogador pode ser feita tentando encaçapar todas as bolinas somente atirando na bolinha branca. Adicionalmente, pode-se estipular que a bola preta deve ser a última a ser encaçapada.


Demonstração:
        https://youtu.be/lF1eAq9VZVc
    

![image](https://github.com/user-attachments/assets/93ec7969-d1d6-4fc9-8d39-8f44d21dbae3)
![image](https://github.com/user-attachments/assets/98a8719b-b5bb-4562-98f7-119c3d0c0900)


  






    
